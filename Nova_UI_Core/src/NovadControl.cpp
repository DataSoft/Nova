//============================================================================
// Name        : NovadControl.cpp
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
// Description : Controls the Novad process itself, in terms of stopping and starting
//============================================================================

#include "MessageManager.h"
#include "Commands.h"
#include "Logger.h"
#include "Lock.h"

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "event2/thread.h"

using namespace Nova;
using namespace std;


pthread_mutex_t messageQueueMutex;
queue<Message_pb*> messageQueue;
pthread_cond_t popWakeupCondition;

extern bool isConnected;
extern struct event_base *libeventBase;
extern struct bufferevent *bufferevent;
extern pthread_mutex_t bufferevent_mutex;

namespace Nova
{

void InitMessaging()
{
	// Make sure config singleton is up
	Config::Inst();

	pthread_mutex_init(&messageQueueMutex, NULL);
	pthread_cond_init(&popWakeupCondition, NULL);

	evthread_use_pthreads();
	libeventBase = event_base_new();
	pthread_mutex_init(&bufferevent_mutex, NULL);

	pthread_t messageWorker;
	pthread_create(&messageWorker, NULL, Nova::ClientMessageWorker, NULL);
	pthread_detach(messageWorker);
}

bool StartNovad(bool blocking)
{
	if(!blocking)
	{
		if(system("nohup novad > /dev/null&") != 0)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		if(system("novad") != 0)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
}

void StopNovad(int32_t messageID)
{
	{
		Lock buffereventLock(&bufferevent_mutex);
		bufferevent_setcb(bufferevent, NULL, MessageManager::WriteDispatcher, NULL, NULL);
	}

	Message_pb killRequest;
	killRequest.set_m_type(CONTROL_EXIT_REQUEST);
	if(messageID != -1)
	{
		killRequest.set_m_messageid(messageID);
	}
	MessageManager::Instance().WriteMessage(&killRequest, 0);
}

bool HardStopNovad()
{
	// THIS METHOD SHOULD ONLY BE CALLED ON DEADLOCK FOR NOVAD
	if(system(string("killall novad").c_str()) != -1)
	{
		LOG(INFO, "Nova has experienced a hard stop", "");
		return true;
	}
	LOG(ERROR, "Something happened while trying to kill Novad", "");
	return false;
}

void SaveAllSuspects(std::string file, int32_t messageID)
{
	Message_pb saveRequest;
	saveRequest.set_m_type(CONTROL_SAVE_SUSPECTS_REQUEST);
	saveRequest.set_m_filepath(file.c_str());
	if(messageID != -1)
	{
		saveRequest.set_m_messageid(messageID);
	}
	MessageManager::Instance().WriteMessage(&saveRequest, 0);
}

void ClearAllSuspects(int32_t messageID)
{
	Message_pb clearRequest;
	clearRequest.set_m_type(CONTROL_CLEAR_ALL_REQUEST);
	if(messageID != -1)
	{
		clearRequest.set_m_messageid(messageID);
	}
	MessageManager::Instance().WriteMessage(&clearRequest, 0);
}

void ClearSuspect(SuspectID_pb suspectId, int32_t messageID)
{
	Message_pb clearRequest;
	clearRequest.set_m_type(CONTROL_CLEAR_SUSPECT_REQUEST);
	*clearRequest.mutable_m_suspectid() = suspectId;
	if(messageID != -1)
	{
		clearRequest.set_m_messageid(messageID);
	}
	MessageManager::Instance().WriteMessage(&clearRequest, 0);
}

void ReclassifyAllSuspects(int32_t messageID)
{
	Message_pb request;
	request.set_m_type(CONTROL_RECLASSIFY_ALL_REQUEST);
	if(messageID != -1)
	{
		request.set_m_messageid(messageID);
	}
	MessageManager::Instance().WriteMessage(&request, 0);
}

void StartPacketCapture(int32_t messageID)
{
	Message_pb request;
	request.set_m_type(CONTROL_START_CAPTURE);
	if(messageID != -1)
	{
		request.set_m_messageid(messageID);
	}
	MessageManager::Instance().WriteMessage(&request, 0);
}

void StopPacketCapture(int32_t messageID)
{
	Message_pb request;
	request.set_m_type(CONTROL_STOP_CAPTURE);
	if(messageID != -1)
	{
		request.set_m_messageid(messageID);
	}
	MessageManager::Instance().WriteMessage(&request, 0);
}

void *ClientMessageWorker(void *arg)
{
	while(true)
	{
		Message_pb *message = MessageManager::Instance().DequeueMessage();

		//If we got a message, then we know we're connected
		//Unless the message is a shutdown message, which we handle below
		isConnected = true;

		//Handle the message in the context of the UI_Core
		switch(message->m_type())
		{
			case UPDATE_SUSPECT:
			{
				break;
			}
			case REQUEST_ALL_SUSPECTS_REPLY:
			{
				break;
			}
			case UPDATE_ALL_SUSPECTS_CLEARED:
			{
				break;
			}
			case UPDATE_SUSPECT_CLEARED:
			{
				break;
			}
			case REQUEST_PONG:
			{
				break;
			}
			case CONNECTION_SHUTDOWN:
			{
				isConnected = false;
				break;
			}
			default:
			{
				break;
			}
		}

		//Hand the message off to the UI for further handling
		Lock lock(&messageQueueMutex);
		messageQueue.push(message);
		pthread_cond_signal(&popWakeupCondition);
	}
	LOG(ERROR, "Messaging worker thread died, should not happen", "");
	return NULL;
}

Message_pb *DequeueUIMessage()
{
	Lock lock(&messageQueueMutex);

	while(messageQueue.empty())
	{
		pthread_cond_wait(&popWakeupCondition, &messageQueueMutex);
	}

	Message_pb *ret = messageQueue.front();
	messageQueue.pop();
	return ret;
}

}
