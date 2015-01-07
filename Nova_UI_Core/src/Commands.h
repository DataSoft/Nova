//============================================================================
// Name        : nova_ui_core.h
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
// Description : Set of command API functions available for use as a UI implementation
//============================================================================

#ifndef COMMANDS_H_
#define COMMANDS_H_

#include <vector>
#include "string"

#include "HaystackControl.h"
#include "Suspect.h"

namespace Nova
{

//************************************************************************
//**						Initialization								**
//************************************************************************

//Must be run first before any other function in the UI_Core
void InitMessaging();

//************************************************************************
//**						Status Queries								**
//************************************************************************

//Runs the Novad process
//	returns - True upon successfully running the novad process, false on error
//	NOTE: This function will return true if Novad was already running
bool StartNovad(bool blocking = false);

//Asks the novad process to exit nicely
void StopNovad(int32_t messageID = -1);

//Kills the Novad process in event of a deadlock
//  returns - True upon killing of Novad process, false on error
bool HardStopNovad();

//Queries Novad to see if it is currently up or down
//	the result is eventually stored such that IsNovadConnected() can retrieve it
void Ping(int32_t messageID = -1);

//Checks if novad is currently up and running
//	this is a local check and does not produce any new messages
bool IsNovadConnected();


//************************************************************************
//**						Connection Operations						**
//************************************************************************

//Initializes a connection out to Novad over IPC
//	NOTE: Must be called before any message can be sent to Novad (but after InitCallbackSocket())
//	returns - true if a successful connection is established, false if no connection (error)
//	NOTE: If a connection already exists, then the function does nothing and returns true
bool ConnectToNovad();

//Disconnects from Novad over IPC. (opposite of ConnectToNovad) Sends no messages
//	NOTE: Safely does nothing if already disconnected
void DisconnectFromNovad();


//************************************************************************
//**						Suspect Operations							**
//************************************************************************

//Asks Novad to save the suspect list to persistent storage
//	returns - true if saved correctly, false on error
void SaveAllSuspects(std::string file, int32_t messageID = -1);

//Asks Novad to forget all data on all suspects that it has
void ClearAllSuspects(int32_t messageID = -1);

//Asks Novad to forget data on the specified suspect
//	suspectAddress - The IP address (unique identifier) of the suspect to forget
void ClearSuspect(SuspectID_pb suspectAddress, int32_t messageID = -1);

//Asks Novad to reclassify all suspects
void ReclassifyAllSuspects(int32_t messageID = -1);

// Asks novad for it's uptime.
void RequestStartTime(int32_t messageID = -1);

// Command nova to start or stop live packet capture
void StartPacketCapture(int32_t messageID = -1);
void StopPacketCapture(int32_t messageID = -1);


//************************************************************************
//**						Event Operations							**
//************************************************************************

//Grabs a message off of the message queue
// Returns - A pointer to a valid Message object. Never NULL. Caller is responsible for life cycle of this message
// NOTE: Blocking call. To be called from worker threads
Message_pb *DequeueUIMessage();

void *ClientMessageWorker(void *arg);

}

#endif /* COMMANDS_H_ */
