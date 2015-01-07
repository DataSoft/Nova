//============================================================================
// Name        : NovaCLI.h
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
// Description : Command line interface for Nova
//============================================================================

#ifndef NOVACLI_H_
#define NOVACLI_H_

#include "protobuf/marshalled_classes.pb.h"

// Name of the CLI executable
#define EXECUTABLE_NAME "novacli"

namespace NovaCLI
{

// Connect to Novad if we can, otherwise print error and exit
void Connect();

void StartNovaWrapper(bool debug);
void StartHaystackWrapper(bool debug);
void StartCaptureWrapper();

void StatusNovaWrapper();
void StatusHaystackWrapper();
bool StatusQuasar();

void StopNovaWrapper();
void StopHaystackWrapper();
void StopCaptureWrapper();

bool StartQuasarWrapper(bool debug);
bool StopQuasarWrapper();

void PrintSuspect(in_addr_t address, std::string interface);
void PrintAllSuspects(enum Nova::SuspectListType listType, bool csv);

void ClearSuspectWrapper(in_addr_t address, std::string interface);
void ClearAllSuspectsWrapper();

void PrintSuspectList(enum Nova::SuspectListType listType);

void PrintUptime();

void PrintUsage();

//Keep waiting for messages until one with the specified message ID arrives
//	NOTE: messageID of -1 indicates to always keep reading messages indefinitely
void MonitorCallback(int32_t messageID = -1);

void ReclassifySuspects();

void ResetPassword();

}

#endif /* NOVACLI_H_ */
