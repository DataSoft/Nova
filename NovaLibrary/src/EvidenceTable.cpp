//============================================================================
// Name        : EvidenceTable.cpp
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
// Description : The EvidenceTable manages a series of EvidenceQueues paired with a suspect's
//				host ordered ipv4 address
//============================================================================

#include "EvidenceTable.h"
#include "Lock.h"
#include <pthread.h>

using namespace std;

namespace Nova
{
	EvidenceTable::EvidenceTable()
	{
		pthread_mutex_init(&m_lock, NULL);
		pthread_cond_init(&m_cond, NULL);
	}

	EvidenceTable::~EvidenceTable()
	{
		pthread_mutex_destroy(&m_lock);
		m_table.clear();
	}

	void EvidenceTable::InsertEvidence(Evidence *evidence)
	{
		Lock lock(&m_lock);
		if(m_table.find(evidence->m_evidencePacket.ip_src) == m_table.end())
		{
			m_table[evidence->m_evidencePacket.ip_src] = GenericQueue<Evidence>();
		}
		//Pushes the evidence and enters the conditional if it's the first piece of evidence
		if(m_table[evidence->m_evidencePacket.ip_src].Push(evidence))
		{
			IpWrapper *temp = new IpWrapper(evidence->m_evidencePacket.ip_src);
			m_processingList.Push(temp);
			//Wake up any consumers waiting for evidence
			pthread_cond_signal(&m_cond);
		}
	}

	Evidence *EvidenceTable::GetEvidence()
	{
		Lock lock(&m_lock);
		IpWrapper *lookup = m_processingList.Pop();
		//While we are unable to get evidence (other consumers got it first)
		while(lookup == NULL)
		{
			//block until evidence is inserted
			pthread_cond_wait(&m_cond, &m_lock);
			lookup = m_processingList.Pop();
		}

		Evidence *ret =  m_table[lookup->ip].PopAll();;
		delete lookup;
		return ret;
	}
}
