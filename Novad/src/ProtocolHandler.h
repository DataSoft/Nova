//============================================================================
// Name        : ProtocolHandler.h
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

#ifndef PROTOCOLHANDLER_H_
#define PROTOCOLHANDLER_H_

#include "Suspect.h"

namespace Nova
{

void HandleExitRequest(Message_pb *incoming);

void HandleClearAllRequest(Message_pb *incoming);

void HandleClearSuspectRequest(Message_pb *incoming);

void HandleReclassifyAllRequest(Message_pb *incoming);

void HandleStartCaptureRequest(Message_pb *incoming);

void HandleStopCaptureRequest(Message_pb *incoming);

void HandleRequestUptime(Message_pb *incoming);

void HandlePing(Message_pb *incoming);

}
#endif /* PROTOCOLHANDLER_H_ */
