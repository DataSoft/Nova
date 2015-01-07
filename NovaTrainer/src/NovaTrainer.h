//============================================================================
// Name        : NovaTrainer.h
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

#ifndef NOVATRAINER_H_
#define NOVATRAINER_H_

#include <pcap.h>
#include <netinet/in.h>

#include "protobuf/marshalled_classes.pb.h"

// Name of the CLI executable
#define EXECUTABLE_NAME "novatrainer"

namespace Nova
{
	enum trainingMode
	{
		trainingMode_capture,
		trainingMode_convert,
		trainingMode_save
	};
	void PrintUsage();

	void SaveToDatabaseFile(std::string captureFolder, std::string databaseFile);

	void ConvertCaptureToDump(std::string captureFolder);

	void CaptureData(std::string captureFolder, std::string interface);
	void SavePacket(u_char *index,const struct pcap_pkthdr *pkthdr,const u_char *packet);

	void SaveAndExit(int param);

	std::string ConstructFilterString();

	void HandleTrainingPacket(u_char *index,const struct pcap_pkthdr *pkthdr,const u_char *packet);
	void UpdateHaystackFeatures();
}


#endif /* NOVATRAINER_H_ */


