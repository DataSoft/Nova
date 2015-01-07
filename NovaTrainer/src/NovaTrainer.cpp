//============================================================================
// Name        : NovaTrainer.cpp
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

#include "ClassificationAggregator.h"
#include "InterfacePacketCapture.h"
#include "ClassificationEngine.h"
#include "FilePacketCapture.h"
#include "HaystackControl.h"
#include "EvidenceTable.h"

#include "DatabaseQueue.h"
#include "Database.h"
#include "NovaTrainer.h"
#include "TrainingDump.h"
#include "Evidence.h"
#include "Suspect.h"
#include "Logger.h"

#include <netinet/if_ether.h>
#include <unistd.h>
#include <vector>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <signal.h>

#define BOOST_FILESYSTEM_VERSION 2
#include <boost/filesystem.hpp>

using namespace std;
using namespace Nova;

ClassificationEngine *engine;
EvidenceTable suspectEvidence;

// Maintains a list of suspects and information on network activity
DatabaseQueue suspects;

pcap_dumper_t *pcapDumpStream;

ofstream trainingFileStream;

trainingMode mode;
string captureFolder;

vector <HoneypotAddress> haystackAddresses;
vector <HoneypotAddress> haystackDhcpAddresses;

int main(int argc, const char *argv[])
{
	if(argc < 2)
	{
		PrintUsage();
	}

    signal(SIGKILL, SaveAndExit);
    signal(SIGINT, SaveAndExit);
    signal(SIGTERM, SaveAndExit);
    signal(SIGPIPE, SIG_IGN);


	if (string(argv[1]) == "--capture")
	{
		mode = trainingMode_capture;
		captureFolder = string(argv[2]);
		string interface = string(argv[3]);

		CaptureData(captureFolder, interface);
	}
	else if (string(argv[1]) == "--convert")
	{
		mode = trainingMode_convert;
		captureFolder = string(argv[2]);

		ConvertCaptureToDump(captureFolder);
	}
	else if (string(argv[1]) == "--save")
	{
		mode = trainingMode_save;
		captureFolder = string(argv[2]);
		string databaseFile = string(argv[3]);

		SaveToDatabaseFile(captureFolder, databaseFile);
	}
	else
	{
		PrintUsage();
	}

	return EXIT_SUCCESS;
}

namespace Nova
{

void PrintUsage()
{
	cout << "Usage:" << endl;
	cout << "  novatrainer --capture novaCaptureFolder interface" << endl;
	cout << "  novatrainer --convert novaCaptureFolder" << endl;
	cout << "  novatrainer --save novaCaptureFolder databaseFile" << endl;

	cout << endl;

	cout << "Steps to generating a training.db file:" << endl;
	cout << "  Create a capture with --capture. Ctrl+c to stop capture. This will create a folder containing capture.pcap and possibly haystackIps.txt." << endl;
	cout << "  Compute all the KNN features from the capture by using --convert. This will generate nova.dump." << endl;
	cout << "  Manually create a file called hostiles.txt. Fill it with the addresses of suspects you want to train as hostile in the capture. One IP per line." << endl;
	cout << "  Finally, run with --save in order to extract lists of benign and hostile IPs into the format needed for the .db file." << endl;
	cout << endl;
	cout << "Example," << endl;
	cout << "  novatrainer --capture /home/test/capture1 eth0" << endl;
	cout << "  Run nmap from some IP 192.168.10.188" << endl;
	cout << "  Ctrl+c the capture." << endl;
	cout << "  novatrainer --convert /home/test/capture1" << endl;
	cout << "  echo \"192.168.10.188\" > /home/tes/capture1/hostiles.txt" << endl;
	cout << "  novatrainer --save /home/test/capture1 training.db" << endl;
	cout << "  Copy desired entries from training.db to the main nova database." << endl;
	cout << endl;

	exit(EXIT_FAILURE);
}

void SaveAndExit(int param)
{
	if (mode == trainingMode_capture)
	{
		pcap_dump_close(pcapDumpStream);
	}

	exit(EXIT_SUCCESS);
}

void HandleTrainingPacket(u_char *index,const struct pcap_pkthdr *pkthdr,const u_char *packet)
{
	if(packet == NULL)
	{
		LOG(ERROR, "Failed to capture packet!","");
		return;
	}
	switch(ntohs(*(uint16_t *)(packet+12)))
	{
		//IPv4, currently the only handled case
		case ETHERTYPE_IP:
		{
			//Prepare Packet structure
			Evidence evidencePacket(packet, pkthdr);

			suspects.ProcessEvidence(&evidencePacket, true);
			return;
		}
		default:
		{
			return;
		}
	}
}

void UpdateHaystackFeatures(string haystackFilePath)
{
	vector<string> ips = Config::GetIpAddresses(haystackFilePath);

	haystackAddresses.empty();
	Database::Inst()->ClearHoneypots();

	for(uint i = 0; i < ips.size(); i++)
	{
		cout << ips[i] << " has been set as a haystack address" << endl;
		Database::Inst()->InsertHoneypotIp(ips[i], "pcap");

		haystackAddresses.push_back(HoneypotAddress(ips[i], "pcap"));
	}

}

string ConstructFilterString()
{
	string filterString = "not src host 0.0.0.0";
	if(Config::Inst()->GetCustomPcapString() != "") {
		if(Config::Inst()->GetOverridePcapString())
		{
			filterString = Config::Inst()->GetCustomPcapString();
			LOG(DEBUG, "Pcap filter string is "+filterString,"");
			return filterString;
		}
		else
		{
			filterString += " && " + Config::Inst()->GetCustomPcapString();
		}
	}

	//Insert static haystack IP's
	vector<HoneypotAddress> hsAddresses = haystackAddresses;
	while(hsAddresses.size())
	{
		//Remove and add the haystack host entry
		filterString += " && not src host ";
		filterString += hsAddresses.back().ip;
		hsAddresses.pop_back();
	}

	// Whitelist the DHCP haystack node IP addresses
	hsAddresses = haystackDhcpAddresses;
	while(hsAddresses.size())
	{
		//Remove and add the haystack host entry
		filterString += " && not src host ";
		filterString += hsAddresses.back().ip;
		hsAddresses.pop_back();
	}

	LOG(DEBUG, "Pcap filter string is "+filterString,"");
	return filterString;
}

void ConvertCaptureToDump(std::string captureFolder)
{
	engine = new ClassificationAggregator();

	Database::Inst()->ClearAllSuspects();

	if(chdir(Config::Inst()->GetPathHome().c_str()) == -1)
	{
		LOG(CRITICAL, "Unable to change folder to " + Config::Inst()->GetPathHome(), "");
	}

	string dumpFile = captureFolder + "/nova.dump";
	string pcapFile = captureFolder + "/capture.pcap";

   	string haystackFile = captureFolder + "/haystackIps.txt";
	UpdateHaystackFeatures(haystackFile);


	trainingFileStream.open(dumpFile);
	if(!trainingFileStream.is_open())
	{
		LOG(CRITICAL, "Unable to open the training capture file.", "Unable to open training capture file at: "+dumpFile);
	}

	FilePacketCapture capture(pcapFile);
	capture.SetPacketCb(HandleTrainingPacket);
	capture.Init();
	capture.SetFilter(ConstructFilterString());
	capture.StartCaptureBlocking();

	LOG(DEBUG, "Done processing PCAP file.", "");

	suspects.WriteToDatabase();

	vector<Suspect> suspects = Database::Inst()->GetSuspects(SUSPECTLIST_ALL);

	for (int i = 0; i < suspects.size(); i++)
	{
		Suspect suspectCopy = suspects[i];

		//Store in training file if needed
		trainingFileStream << suspectCopy.GetIpString() << " ";

		suspectCopy.GetFeatureSet();
		EvidenceAccumulator fs = suspectCopy.GetFeatureSet(MAIN_FEATURES);
		if(fs.m_features[0] != fs.m_features[0] )
		{
			cout << "This can't be good..." << endl;
		}
		for(int j = 0; j < DIM; j++)
		{
			trainingFileStream << fs.m_features[j] << " ";
		}
		trainingFileStream << "\n";
	}


	trainingFileStream.close();
}

void CaptureData(std::string captureFolder, std::string interface)
{
	LOG(DEBUG, "Starting data capture. Storing results in folder:" + captureFolder, "");

	boost::filesystem::path create = captureFolder;

	try
	{
		boost::filesystem::create_directory(create);
	}
	catch(boost::filesystem::filesystem_error const& e)
	{
		LOG(DEBUG, ("Problem creating directory " + captureFolder), ("Problem creating directory " + captureFolder + ": " + e.what()));
	}

    // Write out the state of the haystack at capture
    if(IsHaystackUp())
    {
    	LOG(DEBUG, "Haystack appears up. Recording current state.", "");
    	string haystackFile = captureFolder + "/haystackIps.txt";
        haystackAddresses = Config::GetHaystackAddresses(Config::Inst()->GetPathHome() + "/" + Config::Inst()->GetPathConfigHoneydHS());
        haystackDhcpAddresses = Config::GetHoneydIpAddresses(Config::Inst()->GetIpListPath());

        LOG(DEBUG, "Writing haystack IPs to file " + haystackFile, "");
        ofstream haystackIpStream(haystackFile);
        for(uint i = 0; i < haystackDhcpAddresses.size(); i++)
        {
        	LOG(DEBUG, "Found haystack DHCP IP " + haystackDhcpAddresses.at(i).ip, "");
            haystackIpStream << haystackDhcpAddresses.at(i).ip << endl;
        }
        for(uint i = 0; i < haystackAddresses.size(); i++)
        {
        	LOG(DEBUG, "Found haystack static IP " + haystackAddresses.at(i).ip, "");
            haystackIpStream << haystackAddresses.at(i).ip << endl;
        }

        haystackIpStream.close();
    }

    // Prepare for packet capture
	string trainingCapFile = captureFolder + "/capture.pcap";

    InterfacePacketCapture *capture = new InterfacePacketCapture(interface);
    capture->Init();
    capture->SetPacketCb(SavePacket);

    pcap_t *handle = capture->GetPcapHandle();
    pcap_activate(handle);
    pcapDumpStream = pcap_dump_open(handle, trainingCapFile.c_str());

    capture->StartCaptureBlocking();
 }

void SavePacket(u_char *index,const struct pcap_pkthdr *pkthdr,const u_char *packet)
{
	 pcap_dump((u_char *)pcapDumpStream, pkthdr, packet);
}

void SaveToDatabaseFile(string captureFolder, string databaseFile)
{
	string hostilesFile = captureFolder + "/hostiles.txt";
	vector<string> hostileIps = Config::GetIpAddresses(hostilesFile);

	string dumpFile = captureFolder + "/nova.dump";

	TrainingDump dump;
	dump.LoadCaptureFile(dumpFile);
	dump.SetAllIsHostile(false);
	dump.SetAllIsIncluded(true);

	for (vector<string>::iterator it = hostileIps.begin(); it != hostileIps.end(); it++)
	{
		if (!dump.SetIsHostile(*it, true))
		{
			LOG(ERROR, "hostiles.txt specifies IP " + (*it) + " as hostile, but it does not exist in the capture file.", "");
		}
	}

	dump.MergeIPs(hostileIps, "Hostile IPs from capture: " + captureFolder);
	dump.MergeBenign("Benign IPs from capture: " + captureFolder);
	dump.SaveToDb(databaseFile);
}

}

