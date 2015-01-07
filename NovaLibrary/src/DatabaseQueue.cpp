//============================================================================
// Name        : SuspectTable.cpp
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
// Description : Wrapper class for the SuspectHashMap object used to maintain a
// 		list of suspects.
//============================================================================

#include "ClassificationEngine.h"
#include "DatabaseQueue.h"
#include "HashMap.h"
#include "Config.h"
#include "Logger.h"
#include "Lock.h"
#include "Database.h"

#include <fstream>
#include <sstream>

using namespace std;
using namespace Nova;

extern ClassificationEngine *engine;

namespace Nova
{
DatabaseQueue::DatabaseQueue()
{
	pthread_rwlockattr_t tempAttr;
	pthread_rwlockattr_init(&tempAttr);
	pthread_rwlockattr_setkind_np(&tempAttr,PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
	pthread_rwlock_init(&m_lock, &tempAttr);
}

DatabaseQueue::~DatabaseQueue()
{
	//Deletes the suspects pointed to by the table
	for(SuspectHashTable::iterator it = m_suspectTable.begin(); it != m_suspectTable.end(); it++)
	{
		delete m_suspectTable[it->first];
		m_suspectTable[it->first] = NULL;
	}
	m_suspectTable.clear();

	pthread_rwlock_destroy(&m_lock);
}

bool DatabaseQueue::empty()
{
	return m_suspectTable.size() == 0;
}


//Consumes the linked list of evidence objects, extracting their information and inserting them into the Suspects.
// evidence: Evidence object, if consuming more than one piece of evidence this is the start
//				of the linked list.
// Note: Every evidence object contained in the list is deallocated after use, invalidating the pointers,
//		this is a specialized function designed only for use by Consumer threads.
void DatabaseQueue::ProcessEvidence(Evidence *evidence, bool readOnly)
{
	Lock lock (&m_lock, WRITE_LOCK);
	SuspectID_pb key;
	key.set_m_ip(evidence->m_evidencePacket.ip_src);
	key.set_m_ifname(evidence->m_evidencePacket.interface);

	//Consume and deallocate all the evidence
	//If a suspect already exists
	if(!m_suspectTable.keyExists(key))
	{
		m_suspectTable[key] = new Suspect();
	}

	m_suspectTable[key]->ReadEvidence(evidence, !readOnly);
}


void DatabaseQueue::WriteToDatabase()
{
	Lock lock (&m_lock, WRITE_LOCK);
	int totalCount = 0;

	// This is in a while loop because we break out of the for loop every now and then to keep the
	// queries per transaction down and improve responsiveness of readers
	while (!m_suspectTable.empty())
	{
		Database::Inst()->StartTransaction();
		Database::Inst()->m_count = 0;

		for(SuspectHashTable::iterator it = m_suspectTable.begin(); it != m_suspectTable.end();)
		{
			Suspect *s = it->second;

			Database::Inst()->InsertSuspect(s);
			Database::Inst()->WriteTimestamps(s);

			string ip = s->GetIpString();
			string interface = s->GetInterface();

			Database::Inst()->IncrementPacketCount(ip, interface, s->m_features);

			for(Packet_Table::iterator it = s->m_features.m_packTable.begin() ; it != s->m_features.m_packTable.end(); it++)
			{
				Database::Inst()->IncrementPacketSizeCount(ip, interface, it->first, it->second);
			}

			for (IpPortTable::iterator it = s->m_features.m_hasTcpPortIpBeenContacted.begin(); it != s->m_features.m_hasTcpPortIpBeenContacted.end(); it++)
			{
				Database::Inst()->IncrementPortContactedCount(ip, interface, "tcp", Suspect::GetIpString(it->first.m_ip), it->first.m_port, it->second);
			}

			for (IpPortTable::iterator it = s->m_features.m_hasUdpPortIpBeenContacted.begin(); it != s->m_features.m_hasUdpPortIpBeenContacted.end(); it++)
			{
				Database::Inst()->IncrementPortContactedCount(ip, interface, "udp", Suspect::GetIpString(it->first.m_ip), it->first.m_port, it->second);
			}

			for (IpPortTable::iterator it = s->m_features.m_icmpCodeTypes.begin(); it != s->m_features.m_icmpCodeTypes.end(); it++)
			{
				Database::Inst()->IncrementPortContactedCount(ip, interface, "icmp", Suspect::GetIpString(it->first.m_ip), it->first.m_port, it->second);
			}

			// Random non TCP/UDP packets, we just keep a generic "other" count
			for (IP_Table::iterator it = s->m_features.m_IPTable.begin(); it != s->m_features.m_IPTable.end(); it++)
			{
				Database::Inst()->IncrementPortContactedCount(ip, interface, "other", Suspect::GetIpString(it->first), 0, it->second);
			}


			vector<double> featureset = Database::Inst()->ComputeFeatures(Suspect::GetIpString(it->first), it->first.m_ifname());
			copy(featureset.begin(), featureset.begin() + DIM, it->second->m_features.m_features);

			if (Database::Inst()->GetTotalPacketCount(ip, interface) >= Config::Inst()->GetMinPacketThreshold())
			{
				// Classify the suspect with the new featureset we computed
				engine->Classify(it->second);

				// If it's hostile, see if we need to copy it into the hostile_alerts table
				bool generateHostileAlert = false;
				if (s->GetIsHostile())
				{
					// If it wasn't hostile before, but it is now, make an alert
					if (!Database::Inst()->IsSuspectHostile(s->GetIpString(), s->GetInterface())){
						generateHostileAlert = true;
					}
				}

				// Store result back into database
				Database::Inst()->WriteClassification(s);

				if (generateHostileAlert)
				{
					LOG(ALERT, "Detected potentially hostile traffic from: " + s->ToString(), "");
					Database::Inst()->InsertSuspectHostileAlert(s->GetIpString(), s->GetInterface());

					if(Config::Inst()->GetClearAfterHostile())
					{
						Database::Inst()->ClearSuspect(s->GetIpString(), s->GetInterface());
					}
				}
			}


			it = m_suspectTable.erase(it);
			delete s;

			// Don't do more than 100k queries per transaction. Not a hard and fast rule,
			// but it causes readers to not get new suspects for a long time if we're
			// fuzzing things with the FAST tool, and it doesn't hurt to break things
			// up into multiple transactions if we get really behind processing suspects.
			if (Database::Inst()->m_count > 100000)
			{
				totalCount += Database::Inst()->m_count;
				break;
			}

		}

		Database::Inst()->m_count = totalCount;
		Database::Inst()->StopTransaction();
	}
}

}
