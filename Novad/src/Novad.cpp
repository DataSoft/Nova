//============================================================================
// Name        : Novad.cpp
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
// Description : Nova Daemon to perform network anti-reconnaissance
//============================================================================

#include "HoneydConfiguration/HoneydConfiguration.h"
#include "ClassificationAggregator.h"
#include "InterfacePacketCapture.h"
#include "WhitelistConfiguration.h"
#include "EvidenceAccumulator.h"
#include "FilePacketCapture.h"
#include "HaystackControl.h"
#include "ProtocolHandler.h"
#include "MessageManager.h"
#include "PacketCapture.h"
#include "EvidenceTable.h"
#include "Doppelganger.h"
#include "DatabaseQueue.h"
#include "NovaUtil.h"
#include "Database.h"
#include "Threads.h"
#include "Control.h"
#include "Config.h"
#include "Logger.h"
#include "Point.h"
#include "Novad.h"
#include "Lock.h"

#include <vector>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>
#include <iostream>
#include <ifaddrs.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include "event2/thread.h"
#include <netinet/if_ether.h>

#define BOOST_FILESYSTEM_VERSION 2
#include <boost/filesystem.hpp>

using namespace std;
using namespace Nova;

// Maintains a list of suspects and information on network activity
DatabaseQueue suspects;
//Contains packet evidence yet to be included in a suspect
EvidenceTable suspectEvidence;

pthread_mutex_t packetCapturesLock;
vector<PacketCapture*> packetCaptures;
vector<int> dropCounts;

Doppelganger *doppel;


// Time novad started, used for uptime and pcap capture names
time_t startTime;


//HS Vars
string dhcpListFile;

vector<HoneypotAddress> haystackAddresses;
vector<HoneypotAddress> haystackDhcpAddresses;

vector<string> whitelistIpAddresses;
vector<string> whitelistIpRanges;

int honeydDHCPNotifyFd;
int honeydDHCPWatch;

int whitelistNotifyFd;
int whitelistWatch;

ClassificationEngine *engine = NULL;

pthread_t classificationLoopThread;
pthread_t ipUpdateThread;
pthread_t ipWhitelistUpdateThread;
pthread_t consumer;

pthread_mutex_t shutdownClassificationMutex;
bool shutdownClassification;
pthread_cond_t shutdownClassificationCond;

namespace Nova
{

void StartServer()
{
	int len;
	string inKeyPath = Config::Inst()->GetPathHome() + "/config/keys" + NOVAD_LISTEN_FILENAME;
	evutil_socket_t IPCParentSocket;
	struct event_base *base;
	struct event *listener_event;
	struct sockaddr_un msgLocal;

	startTime = time(NULL);

	evthread_use_pthreads();
	base = event_base_new();
	if (!base)
	{
		LOG(ERROR, "Failed to set up socket base", "");
		return;
	}

	if((IPCParentSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG(ERROR, "Failed to create socket for accept()", "socket: "+string(strerror(errno)));
		return;
	}

	evutil_make_socket_nonblocking(IPCParentSocket);

	msgLocal.sun_family = AF_UNIX;
	memset(msgLocal.sun_path, '\0', sizeof(msgLocal.sun_path));
	strncpy(msgLocal.sun_path, inKeyPath.c_str(), inKeyPath.length());
	unlink(msgLocal.sun_path);
	len = strlen(msgLocal.sun_path) + sizeof(msgLocal.sun_family);

	if(::bind(IPCParentSocket, (struct sockaddr *)&msgLocal, len) == -1)
	{
		LOG(ERROR, "Failed to bind to socket", "bind: "+string(strerror(errno)));
		close(IPCParentSocket);
		return;
	}

	if(listen(IPCParentSocket, SOMAXCONN) == -1)
	{
		LOG(ERROR, "Failed to listen for UIs", "listen: "+string(strerror(errno)));
		close(IPCParentSocket);
		return;
	}

	listener_event = event_new(base, IPCParentSocket, EV_READ|EV_PERSIST, MessageManager::DoAccept, (void*)base);
	event_add(listener_event, NULL);

	//Start our worker threads
	for(int i = 0; i < Config::Inst()->GetNumMessageWorkerThreads(); i++)
	{
		pthread_t workerThread;
		pthread_create(&workerThread, NULL, MessageWorker, NULL);
		pthread_detach(workerThread);
	}

	event_base_dispatch(base);

	LOG(ERROR, "Main accept dispatcher returned. This should not occur.", "");
	return;
}

int RunNovaD()
{
	dhcpListFile = Config::Inst()->GetIpListPath();
	Logger::Inst();
	HoneydConfiguration::Inst();
	Database::Inst();

	if(!LockNovad())
	{
		exit(EXIT_FAILURE);
	}

	// Change our working folder into the config folder so our relative paths are correct
	if(chdir(Config::Inst()->GetPathHome().c_str()) == -1)
	{
		LOG(INFO, "Failed to change directory to " + Config::Inst()->GetPathHome(),"");
	}

	pthread_mutex_init(&packetCapturesLock, NULL);

	//Need to load the configuration before making the Classification Engine for setting up the DM
	//Reload requires a CE object so we do a partial config load here.
	//Loads the configuration file
	Config::Inst()->LoadConfig();

	LOG(ALERT, "Starting NOVA version " + Config::Inst()->GetVersionString(), "");

	doppel = new Doppelganger(suspects);
	doppel->InitDoppelganger();

	engine = new ClassificationAggregator();

	// Set up our signal handlers
	signal(SIGKILL, SaveAndExit);
	signal(SIGINT, SaveAndExit);
	signal(SIGTERM, SaveAndExit);
	signal(SIGPIPE, SIG_IGN);

	haystackAddresses = Config::GetHaystackAddresses(Config::Inst()->GetPathConfigHoneydHS());
	haystackDhcpAddresses = Config::GetHoneydIpAddresses(dhcpListFile);
	whitelistIpAddresses = WhitelistConfiguration::GetIps();
	whitelistIpRanges = WhitelistConfiguration::GetIpRanges();
	UpdateHaystackFeatures();

	whitelistNotifyFd = inotify_init();
	if(whitelistNotifyFd > 0)
	{
		whitelistWatch = inotify_add_watch(whitelistNotifyFd, Config::Inst()->GetPathWhitelistFile().c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
		pthread_create(&ipWhitelistUpdateThread, NULL, UpdateWhitelistIPFilter, NULL);
		pthread_detach(ipWhitelistUpdateThread);
	}
	else
	{
		LOG(ERROR, "Unable to set up file watcher for the Whitelist IP file.","");
	}

	// If we're not reading from a pcap, monitor for IP changes in the honeyd file
	if(!Config::Inst()->GetReadPcap())
	{
		honeydDHCPNotifyFd = inotify_init();

		if(honeydDHCPNotifyFd > 0)
		{
			honeydDHCPWatch = inotify_add_watch (honeydDHCPNotifyFd, dhcpListFile.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
			pthread_create(&ipUpdateThread, NULL, UpdateIPFilter,NULL);
			pthread_detach(ipUpdateThread);
		}
		else
		{
			LOG(ERROR, "Unable to set up file watcher for the honeyd IP list file.","");
		}
	}

	pthread_mutex_init(&shutdownClassificationMutex, NULL);
	shutdownClassification = false;
	pthread_cond_init(&shutdownClassificationCond, NULL);

	if (!Config::Inst()->GetReadPcap()) {
		pthread_create(&classificationLoopThread,NULL,ClassificationLoop, NULL);
		pthread_detach(classificationLoopThread);
	}

	// TODO: Figure out if having multiple Consumer Loops has a performance benefit
	pthread_create(&consumer, NULL, ConsumerLoop, NULL);
	pthread_detach(consumer);

	StartCapture();

	//Go into the main accept() loop
	StartServer();

	return EXIT_FAILURE;
}

bool LockNovad()
{
	int lockFile = open((Config::Inst()->GetPathHome() + "/data/novad.lock").data(), O_CREAT | O_RDWR, 0666);
	int rc = flock(lockFile, LOCK_EX | LOCK_NB);
	if(rc != 0)
	{
		if(errno == EAGAIN)
		{
			cerr << "ERROR: Novad is already running. Please close all other instances before continuing." << endl;
		}
		else
		{
			cerr << "ERROR: Unable to open the novad.lock file. This could be due to bad file permissions on it. Error was: " << strerror(errno) << endl;
		}
		return false;
	}
	else
	{
		// We have the file open, it will be released if the process dies for any reason
		return true;
	}
}

void MaskKillSignals()
{
	sigset_t x;
	sigemptyset (&x);
	sigaddset(&x, SIGKILL);
	sigaddset(&x, SIGTERM);
	sigaddset(&x, SIGINT);
	sigaddset(&x, SIGPIPE);
	sigprocmask(SIG_BLOCK, &x, NULL);
}

void Reload()
{
	// Reload the configuration file
	Config::Inst()->LoadConfig();
	engine->Reload();
}

void StartCapture()
{
	//If we're reading from a packet capture file instead of live capture
	// This is mainly just used for testing and dev work
	if(Config::Inst()->GetReadPcap())
	{
		try
		{
			LOG(DEBUG, "Loading pcap file", "");
			string pcapFilePath = Config::Inst()->GetPathPcapFile() + "/capture.pcap";
			string ipAddressFile = Config::Inst()->GetPathPcapFile() + "/localIps.txt";

			FilePacketCapture *cap = new FilePacketCapture(pcapFilePath.c_str());
			cap->Init();
			cap->SetPacketCb(&Packet_Handler);
			string captureFilterString = ConstructFilterString(cap->GetIdentifier());
			packetCaptures.clear();
			packetCaptures.push_back(cap);
			dropCounts.push_back(0);
			cap->SetFilter(captureFilterString);
			cap->SetIdIndex(packetCaptures.size());

			cap->StartCaptureBlocking();

			LOG(DEBUG, "Done reading pcap file. Processing...", "");
			ClassificationLoop(NULL);
			LOG(DEBUG, "Done processing pcap file.", "");

			exit(EXIT_SUCCESS);
		}
		catch (Nova::PacketCaptureException &e)
		{
			LOG(CRITICAL, string("Unable to open pcap file for capture: ") + e.what(), "");
			exit(EXIT_FAILURE);
		}

		Config::Inst()->SetReadPcap(false); //If we are going to live capture set the flag.
	} else {
		Lock lock(&packetCapturesLock);
		StopCapture_noLocking();

		Reload();


		vector<string> ifList = Config::Inst()->GetInterfaces();

		if (ifList.size() == 0)
		{
			LOG(CRITICAL, "No network interfaces are configured or available for packet capture! "
					"This could be caused by Novad being configured to listen on 'All available interfaces', "
					"but DHCP interfaces haven't yet obtained IP addresses. This could also be caused by a bad configuration. "
					"Please reconfigure the interfaces Novad should listen on.", "");
			exit(EXIT_FAILURE);
		}

		//trainingFileStream = pcap_dump_open(handles[0], trainingCapFile.c_str());

		stringstream temp;
		temp << ifList.size() << endl;

		for(uint i = 0; i < ifList.size(); i++)
		{
			dropCounts.push_back(0);
			InterfacePacketCapture *cap = new InterfacePacketCapture(ifList[i].c_str());

			try
			{
				cap->SetPacketCb(&Packet_Handler);
				cap->Init();
				string captureFilterString = ConstructFilterString(cap->GetIdentifier());
				cap->SetFilter(captureFilterString);
				cap->StartCapture();
				cap->SetIdIndex(packetCaptures.size());
				packetCaptures.push_back(cap);
			}
			catch (Nova::PacketCaptureException &e)
			{
				LOG(CRITICAL, string("Exception when starting packet capture on device " + ifList[i] + ": ") + e.what(), "");
				exit(EXIT_FAILURE);
			}
		}
	}
}

void StopCapture()
{
	Lock lock(&packetCapturesLock);
	StopCapture_noLocking();
}

void StopCapture_noLocking()
{
	for (uint i = 0; i < packetCaptures.size(); i++)
	{
		packetCaptures.at(i)->StopCapture();
		delete packetCaptures.at(i);
	}

	packetCaptures.clear();
}

void Packet_Handler(u_char *index,const struct pcap_pkthdr *pkthdr,const u_char *packet)
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
			Evidence *evidencePacket = new Evidence(packet, pkthdr);
			PacketCapture* cap = reinterpret_cast<PacketCapture*>(index);

			if (cap == NULL)
			{
				LOG(ERROR, "Packet capture object is NULL. Can't tell what interface this packet came from.", "");
				evidencePacket->m_evidencePacket.interface = "UNKNOWN";
			}
			else
			{
				evidencePacket->m_evidencePacket.interface = cap->GetIdentifier();
			}

			if (!Config::Inst()->GetReadPcap())
			{
				suspectEvidence.InsertEvidence(evidencePacket);
			}
			else
			{
				// If reading from pcap file no Consumer threads, so process the evidence right away
				suspects.ProcessEvidence(evidencePacket, false);
			}
			return;
		}
		//Ignore IPV6
		case ETHERTYPE_IPV6:
		{
			return;
		}
		//Ignore ARP
		case ETHERTYPE_ARP:
		{
			return;
		}
		default:
		{
			//stringstream ss;
			//ss << "Ignoring a packet with unhandled protocol #" << (uint16_t)(ntohs(((struct ether_header *)packet)->ether_type));
			//LOG(DEBUG, ss.str(), "");
			return;
		}
	}
}

//Convert monitored ip address into a csv string
string ConstructFilterString(string captureIdentifier)
{
	string filterString = "not src net 0.0.0.0";
	if(Config::Inst()->GetCustomPcapString() != "")
	{
		if(Config::Inst()->GetOverridePcapString())
		{
			filterString = Config::Inst()->GetCustomPcapString();
			LOG(DEBUG, "Pcap filter string is: '" + filterString + "'","");
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
		filterString += " && not src net ";
		filterString += hsAddresses.back().ip;
		hsAddresses.pop_back();
	}

	// Whitelist the DHCP haystack node IP addresses
	hsAddresses = haystackDhcpAddresses;
	while(hsAddresses.size())
	{
		//Remove and add the haystack host entry
		filterString += " && not src net ";
		filterString += hsAddresses.back().ip;
		hsAddresses.pop_back();
	}

	hsAddresses.clear();
	for (uint i = 0; i < whitelistIpAddresses.size(); i++)
	{
		if (WhitelistConfiguration::GetInterface(whitelistIpAddresses.at(i)) == captureIdentifier)
		{
			hsAddresses.push_back(HoneypotAddress(whitelistIpAddresses.at(i), ""));
		}
		else if (WhitelistConfiguration::GetInterface(whitelistIpAddresses.at(i)) == "All Interfaces")
		{
			hsAddresses.push_back(HoneypotAddress(whitelistIpAddresses.at(i), ""));
		}
	}
	while(hsAddresses.size())
	{
		//Remove and add the haystack host entry
		filterString += " && not src net ";
		filterString += WhitelistConfiguration::GetIp(hsAddresses.back().ip);
		hsAddresses.pop_back();
	}

	hsAddresses.clear();
	for (uint i = 0; i < whitelistIpRanges.size(); i++)
	{
		if (WhitelistConfiguration::GetInterface(whitelistIpRanges.at(i)) == captureIdentifier)
		{
			hsAddresses.push_back(HoneypotAddress(whitelistIpRanges.at(i), ""));
		}
		else if (WhitelistConfiguration::GetInterface(whitelistIpRanges.at(i)) == "All Interfaces")
		{
			hsAddresses.push_back(HoneypotAddress(whitelistIpRanges.at(i), ""));
		}
	}
	while(hsAddresses.size())
	{
		filterString += " && not src net ";
		filterString += WhitelistConfiguration::GetIp(hsAddresses.back().ip);
		filterString += " mask ";
		filterString += WhitelistConfiguration::GetSubnet(hsAddresses.back().ip);
		hsAddresses.pop_back();
	}


	// Are we only classifying on honeypot traffic?
	if (Config::Inst()->GetOnlyClassifyHoneypotTraffic())
	{
		hsAddresses = haystackDhcpAddresses;
		hsAddresses.insert(hsAddresses.end(), haystackAddresses.begin(), haystackAddresses.end());
		filterString += " && (0 == 1";
		while(hsAddresses.size())
		{
			filterString += " || dst net ";
			filterString += hsAddresses.back().ip;
			hsAddresses.pop_back();
		}
		filterString += ")";
	}

	LOG(DEBUG, "Pcap filter string is \"" + filterString + "\"","");
	return filterString;
}

void CheckForDroppedPackets()
{
	Lock lock(&packetCapturesLock);
	// Quick check for libpcap dropping packets
	for(uint i = 0; i < packetCaptures.size(); i++)
	{
		int dropped = packetCaptures.at(i)->GetDroppedPackets();
		if(dropped != -1 && dropped != dropCounts[i])
		{
			if(dropped > dropCounts[i])
			{
				stringstream ss;
				ss << "Libpcap has dropped " << dropped - dropCounts[i] << " packets. Try increasing the capture buffer." << endl;
				LOG(WARNING, ss.str(), "");
			}
			dropCounts[i] = dropped;
		}
	}
}

void UpdateHaystackFeatures()
{
	Database::Inst()->StartTransaction();

	for(uint i = 0; i < haystackAddresses.size(); i++)
	{
		Database::Inst()->InsertHoneypotIp(haystackAddresses[i].ip, haystackAddresses[i].interface);
	}

	for(uint i = 0; i < haystackDhcpAddresses.size(); i++)
	{
		Database::Inst()->InsertHoneypotIp(haystackDhcpAddresses[i].ip, haystackDhcpAddresses[i].interface);
	}

	Database::Inst()->StopTransaction();

	stringstream ss;
	ss << "Currently monitoring " << haystackAddresses.size() << " static honeypot IP addresses";
	LOG(DEBUG, ss.str(), "");

	stringstream ss2;
	ss2 << "Currently monitoring " << haystackDhcpAddresses.size() << " DHCP honeypot IP addresses";
	LOG(DEBUG, ss2.str(), "");
}

}
