//============================================================================
// Name        : Connection.cpp
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
// Description : Manages connections out to Novad, initializes and closes them
//============================================================================

#include "MessageManager.h"
#include "Commands.h"
#include "NovaUtil.h"
#include "Logger.h"
#include "Config.h"
#include "Lock.h"

#include <unistd.h>
#include <string>
#include <cerrno>
#include <sys/un.h>
#include <sys/socket.h>
#include "event.h"

using namespace std;
using namespace Nova;

//Socket communication variables
int IPCSocketFD = -1;
bool isConnected = false;
bool shouldDispatch  = false;

struct event_base *libeventBase = NULL;
struct bufferevent *bufferevent = NULL;
pthread_mutex_t bufferevent_mutex;				//Mutex for the bufferevent pointer (not object)
pthread_t eventDispatchThread;

namespace Nova
{

void *EventDispatcherThread(void *arg)
{
	if(!shouldDispatch)
	{
		return NULL;
	}
	int ret = event_base_dispatch(libeventBase);
	if(ret != 0)
	{
		stringstream ss;
		ss << ret;
	}
	else
	{
		LOG(DEBUG, "Message loop ended cleanly.", "");
	}

	DisconnectFromNovad();
	return NULL;
}

bool ConnectToNovad()
{
	if(IsNovadConnected())
	{
		return true;
	}

	DisconnectFromNovad();

	//Builds the key path
	string key = Config::Inst()->GetPathHome();
	key += "/config/keys";
	key += NOVAD_LISTEN_FILENAME;

	//Builds the address
	struct sockaddr_un novadAddress;
	novadAddress.sun_family = AF_UNIX;
	memset(novadAddress.sun_path, '\0', sizeof(novadAddress.sun_path));
	strncpy(novadAddress.sun_path, key.c_str(), sizeof(novadAddress.sun_path));

	{
		Lock buffereventLock(&bufferevent_mutex);
		bufferevent = bufferevent_socket_new(libeventBase, -1,
				BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE |  BEV_OPT_UNLOCK_CALLBACKS | BEV_OPT_DEFER_CALLBACKS );
		if(bufferevent == NULL)
		{
			LOG(ERROR, "Unable to create a socket to Nova", "");
			DisconnectFromNovad();
			return false;
		}

		//Get a new session index and assign it to the bufferevent
		uint32_t *sessionIndex = new uint32_t;
		*sessionIndex = MessageManager::Instance().GetNextSessionIndex();

		bufferevent_setcb(bufferevent, MessageManager::MessageDispatcher, NULL, MessageManager::ErrorDispatcher, sessionIndex);

		if(bufferevent_enable(bufferevent, EV_READ) == -1)
		{
			LOG(ERROR, "Unable to enable socket events", "");
			return false;
		}

		if(bufferevent_socket_connect(bufferevent, (struct sockaddr *)&novadAddress, sizeof(novadAddress)) == -1)
		{
			bufferevent = NULL;
			LOG(DEBUG, "Unable to connect to NOVAD: "+string(strerror(errno))+".", "");
			return false;
		}

		IPCSocketFD = bufferevent_getfd(bufferevent);
		if(IPCSocketFD == -1)
		{
			LOG(DEBUG, "Unable to connect to Novad: "+string(strerror(errno))+".", "");
			bufferevent_free(bufferevent);
			bufferevent = NULL;
			return false;
		}

		if(evutil_make_socket_nonblocking(IPCSocketFD) == -1)
		{
			LOG(DEBUG, "Unable to connect to Novad", "Could not set socket to non-blocking mode");
			bufferevent_free(bufferevent);
			bufferevent = NULL;
			return false;
		}

		MessageManager::Instance().AddSessionIndex(*sessionIndex, bufferevent);
		//LOG(DEBUG, "New connection established", "");
	}

	shouldDispatch = true;
	pthread_create(&eventDispatchThread, NULL, EventDispatcherThread, NULL);

	isConnected = true;
	return true;
}

void DisconnectFromNovad()
{
	//Close out any possibly remaining socket artifacts
	shouldDispatch = false;
	if(libeventBase != NULL)
	{
		if(eventDispatchThread != 0)
		{
			if(event_base_loopbreak(libeventBase) == -1)
			{
				LOG(WARNING, "Unable to exit event loop", "");
			}
			//pthread_join(eventDispatchThread, NULL);
			eventDispatchThread = 0;
		}
	}

	{
		Lock buffereventLock(&bufferevent_mutex);
		if(bufferevent != NULL)
		{
			shutdown(IPCSocketFD, 2);
			bufferevent = NULL;
		}
	}

	IPCSocketFD = -1;
	isConnected = false;
}

}
