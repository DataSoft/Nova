//============================================================================
// Name        : Threads.cpp
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
// Description : Novad thread loops
//============================================================================

#include "WhitelistConfiguration.h"
#include "ClassificationEngine.h"
#include "EvidenceAccumulator.h"
#include "ProtocolHandler.h"
#include "MessageManager.h"
#include "DatabaseQueue.h"
#include "EvidenceTable.h"
#include "PacketCapture.h"
#include "Doppelganger.h"
#include "NovaUtil.h"
#include "Threads.h"
#include "Control.h"
#include "Logger.h"
#include "Config.h"
#include "Point.h"
#include "Novad.h"
#include "Lock.h"
#include "Database.h"

#include <vector>
#include <math.h>
#include <time.h>
#include <string>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/un.h>
#include <net/if.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <netinet/if_ether.h>

#include <arpa/inet.h>

using namespace std;
using namespace Nova;

// Maintains a list of suspects and information on network activity
extern DatabaseQueue suspects;

//HS Vars
extern string dhcpListFile;
extern vector<HoneypotAddress> haystackDhcpAddresses;
extern vector<string> whitelistIpAddresses;
extern vector<string> whitelistIpRanges;
extern vector<PacketCapture*> packetCaptures;

extern int honeydDHCPNotifyFd;
extern int honeydDHCPWatch;

extern int whitelistNotifyFd;
extern int whitelistWatch;

extern pthread_mutex_t packetCapturesLock;

extern EvidenceTable suspectEvidence;

extern Doppelganger *doppel;

extern pthread_mutex_t shutdownClassificationMutex;
extern bool shutdownClassification;
extern pthread_cond_t shutdownClassificationCond;

namespace Nova
{
void *ClassificationLoop(void *ptr)
{
	MaskKillSignals();

	//Classification Loop
	do
	{
		struct timespec timespec;
		struct timeval timeval;
		gettimeofday(&timeval, NULL);
		timespec.tv_sec  = timeval.tv_sec;
		timespec.tv_nsec = timeval.tv_usec*1000;
		timespec.tv_sec += Config::Inst()->GetClassificationTimeout();

		{
			//Protection for the queue structure
			Lock lock(&shutdownClassificationMutex);

			//While loop to protect against spurious wakeups
			while(!shutdownClassification)
			{
				if(pthread_cond_timedwait(&shutdownClassificationCond, &shutdownClassificationMutex, &timespec) == ETIMEDOUT)
				{
					break;
				}
			}
			if(shutdownClassification)
			{
				return NULL;
			}
		}

		CheckForDroppedPackets();

		Database::Inst()->m_count = 0;
		suspects.WriteToDatabase();
		doppel->UpdateDoppelganger();

	}while(Config::Inst()->GetClassificationTimeout() && !Config::Inst()->GetReadPcap());

	if(Config::Inst()->GetReadPcap())
	{
		return NULL;
	}

	//Shouldn't get here!!
	if(Config::Inst()->GetClassificationTimeout())
	{
		LOG(CRITICAL, "The code should never get here, something went very wrong.", "");
	}

	return NULL;
}

void *UpdateIPFilter(void *ptr)
{
	MaskKillSignals();

	while(true)
	{
		if(honeydDHCPWatch > 0)
		{
			int BUF_LEN = (1024 *(sizeof(struct inotify_event)) + 16);
			char buf[BUF_LEN];

			// Blocking call, only moves on when the kernel notifies it that file has been changed
			int readLen = read(honeydDHCPNotifyFd, buf, BUF_LEN);
			if(readLen > 0)
			{
				honeydDHCPWatch = inotify_add_watch(honeydDHCPNotifyFd, dhcpListFile.c_str(),
						IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
				haystackDhcpAddresses = Config::GetHoneydIpAddresses(dhcpListFile);

				UpdateHaystackFeatures();

				{
					Lock lock(&packetCapturesLock);
					for(uint i = 0; i < packetCaptures.size(); i++)
					{
						try {
						string captureFilterString = ConstructFilterString(packetCaptures.at(i)->GetIdentifier());
							packetCaptures.at(i)->SetFilter(captureFilterString);
						}
						catch (Nova::PacketCaptureException &e)
						{
							LOG(ERROR, string("Unable to update capture filter: ") + e.what(), "");
						}
					}
				}
			}
		}
		else
		{
			// This is the case when there's no file to watch, just sleep and wait for it to
			// be created by honeyd when it starts up.
			sleep(2);
			honeydDHCPWatch = inotify_add_watch(honeydDHCPNotifyFd, dhcpListFile.c_str(),
					IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
		}
	}
	return NULL;
}

void *UpdateWhitelistIPFilter(void *ptr)
{
	MaskKillSignals();

	while(true)
	{
		if(whitelistWatch > 0)
		{
			int BUF_LEN = (1024 * (sizeof(struct inotify_event)) + 16);
			char buf[BUF_LEN];

			// Blocking call, only moves on when the kernel notifies it that file has been changed
			int readLen = read(whitelistNotifyFd, buf, BUF_LEN);
			if(readLen > 0)
			{
				whitelistWatch = inotify_add_watch(whitelistNotifyFd, Config::Inst()->GetPathWhitelistFile().c_str(),
						IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
				whitelistIpAddresses = WhitelistConfiguration::GetIps();
				whitelistIpRanges = WhitelistConfiguration::GetIpRanges();

				{
					Lock lock(&packetCapturesLock);
					for(uint i = 0; i < packetCaptures.size(); i++)
					{
						try
						{
							string captureFilterString = ConstructFilterString(packetCaptures.at(i)->GetIdentifier());
							packetCaptures.at(i)->SetFilter(captureFilterString);
						}
						catch (Nova::PacketCaptureException &e)
						{
							LOG(ERROR, string("Unable to update capture filter: ") + e.what(), "");
						}
					}
				}
			}
		}
		else
		{
			// This is the case when there's no file to watch, just sleep and wait for it to
			// be created by honeyd when it starts up.
			sleep(3);
			whitelistWatch = inotify_add_watch(whitelistNotifyFd, Config::Inst()->GetPathWhitelistFile().c_str(),
					IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
		}
	}

	return NULL;
}

void *ConsumerLoop(void *ptr)
{
	while(true)
	{
		//Blocks on a mutex/condition if there's no evidence to process
		Evidence *cur = suspectEvidence.GetEvidence();

		suspects.ProcessEvidence(cur, false);
	}
	return NULL;
}

void *MessageWorker(void *ptr)
{
	while(true)
	{
		Message_pb *message = MessageManager::Instance().DequeueMessage();
		switch(message->m_type())
		{
			case CONTROL_EXIT_REQUEST:
			{
				HandleExitRequest(message);
				break;
			}
			case CONTROL_CLEAR_ALL_REQUEST:
			{
				HandleClearAllRequest(message);
				break;
			}
			case CONTROL_CLEAR_SUSPECT_REQUEST:
			{
				HandleClearSuspectRequest(message);
				break;
			}
			case CONTROL_RECLASSIFY_ALL_REQUEST:
			{
				HandleReclassifyAllRequest(message);
				break;
			}
			case REQUEST_UPTIME:
			{
				HandleRequestUptime(message);
				break;
			}
			case REQUEST_PING:
			{
				HandlePing(message);
				break;
			}
			case CONTROL_START_CAPTURE:
			{
				HandleStartCaptureRequest(message);
				break;
			}
			case CONTROL_STOP_CAPTURE:
			{
				HandleStopCaptureRequest(message);
				break;
			}
			default:
			{
				delete message;
				break;
			}
		}
	}
	return NULL;
}

}
