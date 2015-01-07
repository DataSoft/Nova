//============================================================================
// Name        : NovaCLI.cpp
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

#include "HoneydConfiguration/HoneydConfiguration.h"
#include "nova_ui_core.h"
#include "Database.h"
#include "QuasarDatabase.h"
#include "NovaCLI.h"
#include "Logger.h"
#include "protobuf/marshalled_classes.pb.h"
#include "MessageManager.h"

#include <iostream>
#include <stdlib.h>
#include "inttypes.h"
#include "boost/program_options.hpp"

namespace po = boost::program_options;
using namespace std;
using namespace Nova;
using namespace NovaCLI;

int main(int argc, const char *argv[])
{
	// Fail if no arguments
	if(argc < 2)
	{
		PrintUsage();
	}

	Config::Inst();
	HoneydConfiguration::Inst();

	MessageManager::Instance();
	// Disable notifications and email in the CLI
	Logger::Inst()->SetUserLogPreferences(EMAIL, EMERGENCY, '+');

	InitMessaging();

	// We parse the input arguments here,
	// but refer to other functions to do any
	// actual work.

	// Listing suspect IP addresses
	if(!strcmp(argv[1], "list"))
	{
		if(argc < 3)
		{
			PrintUsage();
		}

		if(!strcmp(argv[2], "all"))
		{
			PrintSuspectList(SUSPECTLIST_ALL);
		}
		else if(!strcmp(argv[2], "hostile"))
		{
			PrintSuspectList(SUSPECTLIST_HOSTILE);
		}
		else if(!strcmp(argv[2], "benign"))
		{
			PrintSuspectList(SUSPECTLIST_BENIGN);
		}
		else
		{
			PrintUsage();
		}
	}

	else if (!strcmp(argv[1], "monitor"))
	{
		MonitorCallback();
	}

	else if (!strcmp(argv[1], "resetpassword"))
	{
		ResetPassword();
	}

	else if (!strcmp(argv[1], "reclassify"))
	{
		ReclassifySuspects();
	}

	// Checking status of components
	else if(!strcmp(argv[1], "status"))
	{
		if(argc < 3)
		{
			PrintUsage();
		}

		if(!strcmp(argv[2], "nova"))
		{
			StatusNovaWrapper();
		}
		else if(!strcmp(argv[2], "haystack"))
		{
			StatusHaystackWrapper();
		}
		else if(!strcmp(argv[2], "quasar"))
		{
			StatusQuasar();
		}
		else
		{
			PrintUsage();
		}
	}

	// Starting components
	else if(!strcmp(argv[1], "start"))
	{
		if(argc < 3)
		{
			PrintUsage();
		}

		if(!strcmp(argv[2], "nova"))
		{
			if (argc > 3)
			{
				if (!strcmp(argv[3], "debug"))
				{
					StartNovaWrapper(true);
				}
				else
				{
					PrintUsage();
				}
			}
			else
			{
				StartNovaWrapper(false);
			}
		}
		else if(!strcmp(argv[2], "haystack"))
		{
			if (argc > 3)
			{
				if (!strcmp(argv[3], "debug"))
				{
					StartHaystackWrapper(true);
				}
				else
				{
					PrintUsage();
				}
			}
			else
			{
				StartHaystackWrapper(false);
			}

		}
		else if(!strcmp(argv[2], "quasar"))
		{
			if (argc > 3)
			{
				if (!strcmp(argv[3], "debug"))
				{
					StartQuasarWrapper(true);
				}
				else
				{
					PrintUsage();
				}
			}
			else
			{
				StartQuasarWrapper(false);
			}

		}
		else if (!strcmp(argv[2], "capture"))
		{
			StartCaptureWrapper();
		}
		else
		{
			PrintUsage();
		}
	}

	// Stopping components
	else if(!strcmp(argv[1], "stop"))
	{
		if(argc < 3)
		{
			PrintUsage();
		}

		if(!strcmp(argv[2], "nova"))
		{
			StopNovaWrapper();
		}
		else if(!strcmp(argv[2], "haystack"))
		{
			StopHaystackWrapper();
		}
		else if(!strcmp(argv[2], "quasar"))
		{
			StopQuasarWrapper();
		}
		else if (!strcmp(argv[2], "capture"))
		{
			StopCaptureWrapper();
		}
		else
		{
			PrintUsage();
		}
	}

	// Getting suspect information
	else if(!strcmp(argv[1], "get"))
	{
		if(argc < 3)
		{
			PrintUsage();
		}

		if(!strcmp(argv[2], "all"))
		{
			if(argc == 4 && !strcmp(argv[3], "csv"))
			{
				PrintAllSuspects(SUSPECTLIST_ALL, true);
			}
			else
			{
				PrintAllSuspects(SUSPECTLIST_ALL, false);
			}
		}
		else if(!strcmp(argv[2], "hostile"))
		{
			if(argc == 4 && !strcmp(argv[3], "csv"))
			{
				PrintAllSuspects(SUSPECTLIST_HOSTILE, true);
			}
			else
			{
				PrintAllSuspects(SUSPECTLIST_HOSTILE, false);
			}
		}
		else if(!strcmp(argv[2], "benign"))
		{
			if(argc == 4 && !strcmp(argv[3], "csv"))
			{
				PrintAllSuspects(SUSPECTLIST_BENIGN, true);
			}
			else
			{
				PrintAllSuspects(SUSPECTLIST_BENIGN, false);
			}
		}
		else
		{
			if (argc != 4)
			{
				PrintUsage();
			}

			// Some early error checking for the
			in_addr_t address;
			if(inet_pton(AF_INET, argv[3], &address) != 1)
			{
				cout << "Error: Unable to convert to IP address" << endl;
				exit(EXIT_FAILURE);
			}

			PrintSuspect(address, string(argv[2]));
		}
	}

	// Clearing suspect information
	else if(!strcmp(argv[1], "clear"))
	{
		if(argc < 3)
		{
			PrintUsage();
		}

		if(!strcmp(argv[2], "all"))
		{
			ClearAllSuspectsWrapper();
		}
		else
		{
			if(argc < 4)
			{
				PrintUsage();
			}

			// Some early error checking for the
			in_addr_t address;
			if(inet_pton(AF_INET, argv[3], &address) != 1)
			{
				cout << "Error: Unable to convert to IP address" << endl;
				exit(EXIT_FAILURE);
			}

			ClearSuspectWrapper(address, string(argv[2]));
		}
	}

	// Checking status of components
	else if(!strcmp(argv[1], "uptime"))
	{
		PrintUptime();
	}


	else if(!strcmp(argv[1], "readsetting"))
	{
		if(argc < 3)
		{
			PrintUsage();
		}

		cout << Config::Inst()->ReadSetting(string(argv[2])) << endl;
	}

	else if(!strcmp(argv[1], "writesetting"))
	{
		if(argc < 4)
		{
			PrintUsage();
		}

		if(Config::Inst()->WriteSetting(string(argv[2]), string(argv[3])))
		{
			LOG(DEBUG, "Finished writing setting to configuration file", "");
		}
		else
		{
			LOG(ERROR, "Unable to write setting to configuration file.", "");
		}
	}

	else if(!strcmp(argv[1], "listsettings"))
	{
		vector<string> settings = Config::Inst()->GetPrefixes();
		for (uint i = 0; i < settings.size(); i++)
		{
			cout << settings[i] << endl;
		}
	}

	else
	{
		PrintUsage();
	}

	return EXIT_SUCCESS;
}

namespace NovaCLI
{

void PrintUsage()
{
	cout << "Usage:" << endl;
	cout << "  " << EXECUTABLE_NAME << " status nova|haystack|quasar" << endl;
	cout << "    Outputs if the nova or haystack process is running and responding" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " start nova|capture|haystack|quasar [debug]" << endl;
	cout << "    Starts the nova or haystack process, or starts capture on existing process. 'debug' will run in a blocking and verbose mode." << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " stop nova|capture|haystack|quasar" << endl;
	cout << "    Stops the nova, haystack process, or live packet capture" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " list all|hostile|benign" << endl;
	cout << "    Outputs the current list of suspect IP addresses of a given type" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " get all|hostile|benign [csv]" << endl;
	cout << "    Outputs the details for all suspects of a type (all, hostile only, or benign only). Optionally can be output in CSV format." << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " get interface xxx.xxx.xxx.xxx" << endl;
	cout << "    Outputs the details of a specific suspect with IP address xxx.xxx.xxx.xxx" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " get data interface xxx.xxx.xxx.xxx" << endl;
	cout << "    Outputs the details of a specific suspect with IP address xxx.xxx.xxx.xxx, including low level data" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " clear all" << endl;
	cout << "    Clears all saved data for suspects" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " clear interface xxx.xxx.xxx.xxx" << endl;
	cout << "    Clears all saved data for a specific suspect" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " writesetting SETTING VALUE" << endl;
	cout << "    Writes setting to configuration file" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " readsetting SETTING" << endl;
	cout << "    Reads setting from configuration file" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " listsettings" << endl;
	cout << "    Lists settings that can be set in the configuration file" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " monitor" << endl;
	cout << "    Monitors live output from novad (mainly for debugging)" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " resetpassword" << endl;
	cout << "    Reset the Quasar password to nova/toor (add nova user if doesn't exist, otherwise change password to 'toor')" << endl;

	exit(EXIT_FAILURE);
}

void StatusNovaWrapper()
{
	if(!ConnectToNovad())
	{
		cout << "Novad Status: Not running" << endl;
	}
	else
	{
		Ping(1);
		MonitorCallback(1);

		if(IsNovadConnected())
		{
			cout << "Novad Status: Running and responding" << endl;
		}
		else
		{
			cout << "Novad Status: Not responding" << endl;
		}

		DisconnectFromNovad();
	}
}

void StatusHaystackWrapper()
{
	if(IsHaystackUp())
	{
		cout << "Haystack Status: Running" << endl;
	}
	else
	{
		cout << "Haystack Status: Not running" << endl;
	}
}

bool StatusQuasar()
{
	return system("forever list");
}

void StartNovaWrapper(bool debug)
{
	if(!ConnectToNovad())
	{
		if(StartNovad(debug))
		{
			cout << "Started Novad" << endl;
		}
		else
		{
			cout << "Failed to start Novad" << endl;
		}
	}
	else
	{
		//Verify that the connection is good
		Ping(1);
		MonitorCallback(1);

		if(IsNovadConnected())
		{
			cout << "Novad is already running" << endl;
			DisconnectFromNovad();
		}
		else
		{
			if(StartNovad(debug))
			{
				cout << "Started Novad" << endl;
			}
			else
			{
				cout << "Failed to start Novad" << endl;
			}
		}
	}
}

void StartHaystackWrapper(bool debug)
{
	if(!IsHaystackUp())
	{
		if(StartHaystack(debug))
		{
			cout << "Started Haystack" << endl;
		}
		else
		{
			cout << "Failed to start Haystack" << endl;
		}
	}
	else
	{
		cout << "Haystack is already running" << endl;
	}
}

void StopNovaWrapper()
{
	Connect();
	Ping(1);
	MonitorCallback(1);
	if(IsNovadConnected())
	{
		StopNovad();
	}
	else
	{
		cout << "Unable to connect to novad" << endl;
	}
}

void StopHaystackWrapper()
{
	if(StopHaystack())
	{
		cout << "Haystack has been stopped" << endl;
	}
	else
	{
		cout << "There was a problem stopping the Haystack" << endl;
	}
}

void StartCaptureWrapper()
{
	Connect();
	StartPacketCapture(1);
	MonitorCallback(1);
}

void StopCaptureWrapper()
{
	Connect();
	StopPacketCapture(1);
	MonitorCallback(1);
}

bool StartQuasarWrapper(bool debug)
{
	if(StopQuasarWrapper())
	{
		return system("quasar --debug");
	}
	else
	{
		return system("quasar");
	}
}

bool StopQuasarWrapper()
{
	return system("forever stop /usr/share/nova/sharedFiles/Quasar/main.js");
}


void PrintSuspect(in_addr_t address, string interface)
{
	SuspectID_pb id;
	id.set_m_ifname(interface);
	id.set_m_ip(ntohl(address));

	Suspect s = Database::Inst()->GetSuspect(id);

	if (s.GetIpAddress() != 0)
	{
		cout << s.ToString() << endl;
	}
	else
	{
		cout << "Could not fetch that suspect" << endl;
	}
}

void PrintAllSuspects(enum SuspectListType listType, bool csv)
{
	vector<Suspect> suspects = Database::Inst()->GetSuspects(listType);

	// Print the CSV header
	if(csv)
	{
		cout << "IP,";
		cout << "INTERFACE,";
		cout << "CLASSIFICATION" << ",";

		for(int i = 0; i < DIM; i++)
		{
			cout << EvidenceAccumulator::m_featureNames[i] << ",";
		}

		cout << endl;

		for (uint i = 0; i < suspects.size(); i++)
		{
			cout << suspects[i].GetIpString() << ",";
			cout << suspects[i].GetInterface() << ",";
			cout << suspects[i].GetClassification() << ",";

			for (uint j = 0; j < DIM; j++)
			{
				cout << suspects[i].m_features.m_features[j] << ",";
			}

			cout << endl;
		}
	}
	else
	{
		for (uint i = 0; i < suspects.size(); i++)
		{
			cout << suspects[i].ToString() << endl;
		}
	}
}

void PrintSuspectList(enum SuspectListType listType)
{
	vector<string> suspects = Database::Inst()->GetSuspectList(listType);
	for (uint i = 0 ; i < suspects.size(); i++)
	{
		cout << suspects[i] << endl;
	}
}

void ClearAllSuspectsWrapper()
{
	Connect();
	ClearAllSuspects(1);
	MonitorCallback(1);
	DisconnectFromNovad();
}

void ClearSuspectWrapper(in_addr_t address, string interface)
{
	Connect();

	SuspectID_pb id;
	id.set_m_ifname(interface);
	id.set_m_ip(ntohl(address));

	ClearSuspect(id, 1);
	MonitorCallback(1);
	DisconnectFromNovad();
}

void PrintUptime()
{
	Connect();
	RequestStartTime(1);
	MonitorCallback(1);
	DisconnectFromNovad();
}

void Connect()
{
	if(!ConnectToNovad())
	{
		cout << "ERROR: Unable to connect to Nova" << endl;
		exit(EXIT_FAILURE);
	}
}

void ReclassifySuspects()
{
	Connect();
	ReclassifyAllSuspects(1);

	MonitorCallback(1);
	DisconnectFromNovad();
}

void ResetPassword()
{
	if (QuasarDatabase::Inst()->ResetPassword() != -1) {
		LOG(ALERT, "Username/password has been reset to nova/toor by the command line Nova interface", "");
	} else {
		LOG(ERROR, "Error: something went wrong when trying to reset the password", "");
	}
}

void MonitorCallback(int32_t messageID)
{
    if(!Nova::ConnectToNovad())
    {
    	LOG(ERROR, "CLI Unable to connect to Novad","");
    	return;
    }

	while(true)
	{
		Message_pb *message = DequeueUIMessage();

    	//If the connection shut down, then just quit no matter what
    	if(message->m_type() == CONNECTION_SHUTDOWN)
    	{
    		cout << "Connection Terminated" << endl;
    		delete message;
    		return;
    	}

    	//Only process a message if it was the one we're looking for OR if we're watching them all
    	if((messageID == -1) || (message->has_m_messageid() && (message->m_messageid() == messageID)))
    	{
    		switch(message->m_type())
    		{
    			case UPDATE_ALL_SUSPECTS_CLEARED:
    			{
					cout << "All suspects were cleared" << endl;
    				break;
    			}
    			case UPDATE_SUSPECT_CLEARED:
    			{
    				if(message->m_success())
    				{
    					cout << "Suspect " << Suspect::GetIpString(message->m_suspectid()) << " was cleared" << endl;
    				}
    				else
    				{
    					cout << "Failed to clear Suspect " << Suspect::GetIpString(message->m_suspectid()) << endl;
    				}
    				break;
    			}
    			case REQUEST_PONG:
    			{
    				cout << "Pong" << endl;
    				break;
    			}
    			case REQUEST_UPTIME_REPLY:
    			{
    				time_t startTime = message->m_starttime();
    				char buff[30];
    				strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", localtime(&startTime));
    				cout << "Novad has been running since: " << buff << endl;
    				break;
    			}
    			default:
    			{
    				break;
    			}
    		}
    	}
	    //If we're waiting only for a specific message, and it has arrived, then quit
    	if((messageID != -1) && message->has_m_messageid() && (message->m_messageid() == messageID))
		{
    		cout << "Connection Terminated" << endl;
			delete message;
			return;
		}

		delete message;
	}
}

}
