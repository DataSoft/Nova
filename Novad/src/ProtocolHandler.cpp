//============================================================================
// Name        : ProtocolHandler.cpp
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
// Description : Manages the message sending protocol to and from the Nova UI
//============================================================================

#include "Database.h"
#include "ProtocolHandler.h"
#include "MessageManager.h"
#include "Config.h"
#include "Logger.h"
#include "Control.h"
#include "Novad.h"
#include "Lock.h"

#include <sstream>
#include "pthread.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cerrno>
#include <stdio.h>
#include <stdlib.h>

#define BOOST_FILESYSTEM_VERSION 2
#include <boost/filesystem.hpp>

using namespace Nova;
using namespace std;

int IPCParentSocket = -1;

extern time_t startTime;

struct sockaddr_un msgRemote, msgLocal;
int UIsocketSize;

namespace Nova
{

void HandleExitRequest(Message_pb *incoming)
{
	LOG(NOTICE, "Quitting: Got an exit request from the UI. Goodbye!",
			"Got a CONTROL_EXIT_REQUEST, quitting.");
	SaveAndExit(0);
}

void HandleClearAllRequest(Message_pb *incoming)
{
	Database::Inst()->StartTransaction();
	Database::Inst()->ClearAllSuspects();
	Database::Inst()->StopTransaction();

	LOG(DEBUG, "Cleared all suspects due to UI request",
			"Got a CONTROL_CLEAR_ALL_REQUEST, cleared all suspects.");

	//First, send the reply (wth message ID) to just the original sender
	Message_pb updateMessage;
	updateMessage.set_m_type(UPDATE_ALL_SUSPECTS_CLEARED);
	if(incoming->has_m_messageid())
	{
		updateMessage.set_m_messageid(incoming->m_messageid());
	}
	MessageManager::Instance().WriteMessage(&updateMessage, incoming->m_sessionindex());

	//Now send a generic message to the rest of the clients
	updateMessage.clear_m_messageid();
	MessageManager::Instance().WriteMessageExcept(&updateMessage, incoming->m_sessionindex());
}

void HandleClearSuspectRequest(Message_pb *incoming)
{
	struct in_addr suspectAddress;
	suspectAddress.s_addr = ntohl(incoming->m_suspectid().m_ip());

	Database::Inst()->StartTransaction();
	Database::Inst()->ClearSuspect(string(inet_ntoa(suspectAddress)), incoming->m_suspectid().m_ifname());
	Database::Inst()->StopTransaction();

	LOG(DEBUG, "Cleared a suspect due to UI request",
			"Got a CONTROL_CLEAR_SUSPECT_REQUEST, cleared suspect: "
			+ string(inet_ntoa(suspectAddress)) + " on interface " + incoming->m_suspectid().m_ifname() + ".");

	//First, send the reply (wth message ID) to just the original sender
	Message_pb updateMessage;
	updateMessage.set_m_type(UPDATE_SUSPECT_CLEARED);
	updateMessage.set_m_success(true);
	if(incoming->has_m_messageid())
	{
		updateMessage.set_m_messageid(incoming->m_messageid());
	}
	updateMessage.mutable_m_suspectid()->CopyFrom(incoming->m_suspectid());
	MessageManager::Instance().WriteMessage(&updateMessage, 0);

	//Now send a generic message to the rest of the clients
	updateMessage.clear_m_messageid();
	MessageManager::Instance().WriteMessageExcept(&updateMessage, incoming->m_sessionindex());
}

void HandleReclassifyAllRequest(Message_pb *incoming)
{
	Reload();

	LOG(DEBUG, "Reclassified all suspects due to UI request",
		"Got a CONTROL_RECLASSIFY_ALL_REQUEST, reclassified all suspects.");
}

void HandleStartCaptureRequest(Message_pb *incoming)
{
	StartCapture();
}

void HandleStopCaptureRequest(Message_pb *incoming)
{
	StopCapture();
}

void HandleRequestUptime(Message_pb *incoming)
{
	Message_pb reply;
	reply.set_m_type(REQUEST_UPTIME_REPLY);
	if(incoming->has_m_messageid())
	{
		reply.set_m_messageid(incoming->m_messageid());
	}
	reply.set_m_starttime(startTime);
	MessageManager::Instance().WriteMessage(&reply, incoming->m_sessionindex());
}

void HandlePing(Message_pb *incoming)
{
	Message_pb pong;
	if(incoming->has_m_messageid())
	{
		pong.set_m_messageid(incoming->m_messageid());
	}
	pong.set_m_type(REQUEST_PONG);
	MessageManager::Instance().WriteMessage(&pong, incoming->m_sessionindex());
}

}
