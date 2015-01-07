//============================================================================
// Name        : MessageManager.cpp
// Copyright   : DataSoft Corporation 2011-2013
//	Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Manages all incoming messages on sockets
//============================================================================

#include "MessageManager.h"
#include "Lock.h"
#include "Logger.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include "event2/thread.h"

#include <sstream>

using namespace std;

namespace Nova
{

MessageManager *MessageManager::m_instance = NULL;

MessageManager::MessageManager()
{
	pthread_mutex_init(&m_queueMutex, NULL);
	pthread_mutex_init(&m_sessionIndexMutex, NULL);
	pthread_mutex_init(&m_bevMapMutex, NULL);
	pthread_cond_init(&m_popWakeupCondition, NULL);
	m_sessionIndex = 1;
}

MessageManager &MessageManager::Instance()
{
	if(m_instance == NULL)
	{
		m_instance = new MessageManager();
	}
	return *MessageManager::m_instance;
}

Message_pb *MessageManager::DequeueMessage()
{
	Lock lock(&m_queueMutex);

	while(m_queue.empty())
	{
		pthread_cond_wait(&m_popWakeupCondition, &m_queueMutex);
	}

	Message_pb *ret = m_queue.front();
	m_queue.pop();
	return ret;
}

void MessageManager::EnqueueMessage(Message_pb *message)
{
	Lock lock(&m_queueMutex);
	m_queue.push(message);
	pthread_cond_signal(&m_popWakeupCondition);
}

bool MessageManager::WriteMessage(Message_pb *message, uint32_t sessionIndex)
{
	if(message == NULL)
	{
		return false;
	}

	if(sessionIndex != 0)
	{
		Lock lock(&m_bevMapMutex);
		if(m_bevMap.count(sessionIndex) == 0)
		{
			return false;
		}
		struct bufferevent *bev = m_bevMap[sessionIndex];

		uint32_t length = message->ByteSize();
		char *buffer = new char[length + sizeof(uint32_t)];

		memcpy(buffer, &length, sizeof(length));

		if(!message->SerializeToArray(buffer + sizeof(uint32_t), length))
		{
			delete[] buffer;
			return false;
		}
		bufferevent_lock(bev);
		if(bufferevent_write(bev, buffer, length + sizeof(uint32_t)) == -1)
		{
			delete[] buffer;
			bufferevent_unlock(bev);
			return false;
		}

		delete[] buffer;
		bufferevent_unlock(bev);
		return true;
	}
	else
	{
		Lock lock(&m_bevMapMutex);
		if(m_bevMap.empty())
		{
			return false;
		}
		map<uint32_t, struct bufferevent*>::iterator it;
		for(it = m_bevMap.begin(); it != m_bevMap.end(); it++)
		{
			struct bufferevent *bev = it->second;

			if(bev == NULL)
			{
				continue;
			}

			uint32_t length = message->ByteSize();
			char *buffer = new char[length + sizeof(uint32_t)];

			memcpy(buffer, &length, sizeof(length));

			if(!message->SerializeToArray(buffer + sizeof(uint32_t), length))
			{
				delete[] buffer;
				continue;
			}
			bufferevent_lock(bev);
			if(bufferevent_write(bev, buffer, length + sizeof(uint32_t)) == -1)
			{
				delete[] buffer;
				bufferevent_unlock(bev);
				continue;
			}

			delete[] buffer;
			bufferevent_unlock(bev);
		}
		return true;
	}
}

bool MessageManager::WriteMessageExcept(Message_pb *message, uint32_t sessionIndex)
{
	if(message == NULL)
	{
		return false;
	}
	if(sessionIndex == 0)
	{
		return false;
	}

	Lock lock(&m_bevMapMutex);
	if(m_bevMap.empty())
	{
		return false;
	}

	map<uint32_t, struct bufferevent*>::iterator it;
	for(it = m_bevMap.begin(); it != m_bevMap.end(); it++)
	{
		struct bufferevent *bev = it->second;

		if((it->first == sessionIndex) || (bev == NULL))
		{
			continue;
		}

		uint32_t length = message->ByteSize();
		char *buffer = new char[length + sizeof(uint32_t)];

		memcpy(buffer, &length, sizeof(length));

		if(!message->SerializeToArray(buffer + sizeof(uint32_t), length))
		{
			delete[] buffer;
			continue;
		}
		bufferevent_lock(bev);
		if(bufferevent_write(bev, buffer, length + sizeof(uint32_t)) == -1)
		{
			delete[] buffer;
			bufferevent_unlock(bev);
			continue;
		}

		delete[] buffer;
		bufferevent_unlock(bev);
	}
	return true;
}

void MessageManager::WriteDispatcher(struct bufferevent *bev, void *ctx)
{
	bufferevent_lock(bev);

	struct evbuffer *output = bufferevent_get_output(bev);
	uint32_t evbufferLength = evbuffer_get_length(output);

	bufferevent_unlock(bev);

	if(evbufferLength == 0)
	{
		Message_pb *shutdown = new Message_pb();
		shutdown->set_m_type(CONNECTION_SHUTDOWN);
		MessageManager::Instance().EnqueueMessage(shutdown);
	}
}

void MessageManager::MessageDispatcher(struct bufferevent *bev, void *ctx)
{
	bool keepGoing = true;
	while(keepGoing)
	{
		bufferevent_lock(bev);

		struct evbuffer *input = bufferevent_get_input(bev);

		uint32_t length = 0;
		uint32_t evbufferLength = evbuffer_get_length(input);

		//If we don't even have enough data to read the length, just quit
		if(evbufferLength < sizeof(length))
		{
			keepGoing = false;
			continue;
		}

		//Copy the length field out of the message
		//	We only want to copy this data at first, because if the whole message hasn't reached us,
		//	we'll want the whole buffer still present here, undrained
		if(evbuffer_copyout(input, &length, sizeof(length)) != sizeof(length))
		{
			LOG(ERROR, "evbuffer_copyout failed", "");
			keepGoing = false;
			continue;
		}

		//If we don't yet have enough data, then just quit and wait for more
		if(evbufferLength < (length + sizeof(length)))
		{
			keepGoing = false;
			continue;
		}

		//Remove the length from the buffer
		int res = evbuffer_drain(input, sizeof(length));
		if (res != 0)
		{
			LOG(ERROR, "Error draining the event buffer. This shouldn't happen.", "");
			keepGoing = false;
			continue;
		}

		char *buffer = (char*)evbuffer_pullup(input, length);
		if(buffer == NULL)
		{
			// This should never happen. If it does, probably because length is an absurd value (or we're out of memory)
			LOG(ERROR, "Error getting libevent data in message read", "");
			keepGoing = false;
			continue;
		}

		Message_pb *message = new Message_pb();
		if(message->ParseFromArray(buffer, length) == true)
		{
			uint32_t *index = (uint32_t*)ctx;
			message->set_m_sessionindex(*index);
			MessageManager::Instance().EnqueueMessage(message);
		}
		else
		{
			delete message;
		}

		res = evbuffer_drain(input, length);
		if (res != 0)
		{
			LOG(ERROR, "Error draining the event buffer. This shouldn't happen.", "");
		}
		bufferevent_unlock(bev);
	}

	bufferevent_unlock(bev);
}

void MessageManager::ErrorDispatcher(struct bufferevent *bev, short error, void *ctx)
{
	if(error & BEV_EVENT_CONNECTED)
	{
		return;
	}

	//If the socket has closed, clean up the bufferevent
	if(error & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
	{
		//LOG(DEBUG, "Connection has terminated", "");
		uint32_t *index = (uint32_t*)ctx;
		MessageManager::Instance().RemoveSessionIndex(*index);

		Message_pb *shutdown = new Message_pb();
		shutdown->set_m_type(CONNECTION_SHUTDOWN);
		MessageManager::Instance().EnqueueMessage(shutdown);
		delete index;
	}
}

void MessageManager::DoAccept(evutil_socket_t listener, short event, void *arg)
{
	LOG(DEBUG, "Connected to client", "");

    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(listener, (struct sockaddr*)&ss, &slen);
    if(fd < 0)
    {
    	LOG(ERROR, "Failed to connect to UI", "accept: " + string(strerror(errno)));
    }
    else
    {
		struct bufferevent *bev;
		evutil_make_socket_nonblocking(fd);
		bev = bufferevent_socket_new((event_base*)arg, fd,
				BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE |  BEV_OPT_UNLOCK_CALLBACKS | BEV_OPT_DEFER_CALLBACKS );
		if(bev == NULL)
		{
			LOG(ERROR, "Failed to connect to UI: socket_new", "");
			return;
		}

		//Get a new session index and assign it to the bufferevent
		uint32_t *sessionIndex = new uint32_t;
		*sessionIndex = MessageManager::Instance().GetNextSessionIndex();
		MessageManager::Instance().AddSessionIndex(*sessionIndex, bev);

		bufferevent_setcb(bev, MessageDispatcher, NULL, ErrorDispatcher, sessionIndex);
		bufferevent_setwatermark(bev, EV_READ, 0, 0);
		if(bufferevent_enable(bev, EV_READ|EV_WRITE) == -1)
		{
			LOG(ERROR, "Failed to connect to UI: bufferevent_enable", "");
			return;
		}
    }
}

uint32_t MessageManager::GetNextSessionIndex()
{
	Lock lock(&m_sessionIndexMutex);
	return m_sessionIndex++;
}

void MessageManager::RemoveSessionIndex(uint32_t index)
{
	Lock lock(&m_bevMapMutex);
	if(m_bevMap.count(index) > 0)
	{
		struct bufferevent *bev = m_bevMap[index];
		bufferevent_free(bev);
		m_bevMap.erase(index);
	}
}

void MessageManager::AddSessionIndex(uint32_t index, struct bufferevent *bev)
{
	Lock lock(&m_bevMapMutex);
	if(m_bevMap.count(index) > 0)
	{
		LOG(WARNING, "An old bufferevent is stuck, this shouldn't be. Clobbering it.", "");
	}
	m_bevMap[index] = bev;
}

}
