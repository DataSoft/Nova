//============================================================================
// Name        : EvidenceTable.h
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

#ifndef EVIDENCETABLE_H_
#define EVIDENCETABLE_H_

#include "HashMapStructs.h"
#include "GenericQueue.h"
#include "Evidence.h"

typedef Nova::HashMap<uint64_t, Nova::GenericQueue<Nova::Evidence>, std::hash<uint64_t>, eqkey > EvidenceHashTable;

namespace Nova
{

class EvidenceTable
{

public:
	EvidenceTable();
	~EvidenceTable();

	//Inserts the Evidence into the table at the location specified by the destination address
	void InsertEvidence(Evidence *packet);

	// Returns the first evidence object in a Evidence linked list or NULL if no evidence for any entries
	// After use each Evidence object must be explicitly deallocated
	Evidence *GetEvidence();

private:

	// This is a FIFO list of suspect IP addresses that we have evidence for and need processing
	GenericQueue<IpWrapper> m_processingList;

	// Contains evidence for the suspects, keyed by source IP address of the packets
	// May contain multiple chunks of evidence per suspect (stored with a GenericQueue)
	EvidenceHashTable m_table;


	pthread_mutex_t m_lock;
	pthread_cond_t m_cond;
};

}

#endif /* EVIDENCETABLE_H_ */
