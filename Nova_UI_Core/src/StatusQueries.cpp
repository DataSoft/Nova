//============================================================================
// Name        : StatusQueries.cpp
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
// Description : Handles requests for information from Novad
//============================================================================

#include "Commands.h"
#include "MessageManager.h"
#include "Logger.h"
#include "Lock.h"

#include <iostream>

using namespace Nova;
using namespace std;

extern bool isConnected;

namespace Nova
{

bool IsNovadConnected()
{
	return isConnected;
}

void Ping(int32_t messageID)
{
	Message_pb ping;
	ping.set_m_type(REQUEST_PING);
	if(messageID != -1)
	{
		ping.set_m_messageid(messageID);
	}
	MessageManager::Instance().WriteMessage(&ping, 0);
}

void RequestStartTime(int32_t messageID)
{
	Message_pb getUptime;
	getUptime.set_m_type(REQUEST_UPTIME);
	if(messageID != -1)
	{
		getUptime.set_m_messageid(messageID);
	}
	MessageManager::Instance().WriteMessage(&getUptime, 0);
}

}
