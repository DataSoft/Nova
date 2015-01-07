// ============================================================================
// Name        : SuspectTable.h
// Copyright   : DataSoft Corporation 2011-2013
// 	Nova is free software: you can redistribute it and/or modify
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
//   along with Nova.  If not, see <http:// www.gnu.org/licenses/>.
// Description : Wrapper class for the SuspectHashMap object used to maintain a
// 		list of suspects.
// ============================================================================

#ifndef SUSPECTTABLE_H_
#define SUSPECTTABLE_H_

#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <vector>

#include "Suspect.h"
#include "protobuf/marshalled_classes.pb.h"

namespace std
{
	template<>
	struct hash< Nova::SuspectID_pb >
	{
		std::size_t operator()( const Nova::SuspectID_pb &c ) const
		{
			return hash<uint32_t>()(c.m_ip());
		}
	};
}

namespace Nova
{

struct SuspectIDEq
{
	bool operator()(Nova::SuspectID_pb k1, Nova::SuspectID_pb k2) const
	{
		if((k1.m_ip() == k2.m_ip()) && (k1.m_ifname() == k2.m_ifname()))
		{
			return true;
		}
		return false;
	}
};

typedef Nova::HashMap<Nova::SuspectID_pb, Nova::Suspect *, std::hash<Nova::SuspectID_pb>, Nova::SuspectIDEq> SuspectHashTable;


class DatabaseQueue
{

public:
	DatabaseQueue();
	~DatabaseQueue();

	bool empty();

	//Consumes the linked list of evidence objects, extracting their information and inserting them into the Suspects.
	// evidence: Evidence object, if consuming more than one piece of evidence this is the start
	//				of the linked list.
	// Note: Every evidence object contained in the list is deallocated after use, invalidating the pointers,
	//		this is a specialized function designed only for use by Consumer threads.
	void ProcessEvidence(Evidence *evidence, bool readOnly = false);

	void WriteToDatabase();
private:

	// Hashmap used for constant time key lookups
	SuspectHashTable m_suspectTable;

	pthread_rwlock_t m_lock;
};

}

#endif /* SUSPECTTABLE_H_ */
