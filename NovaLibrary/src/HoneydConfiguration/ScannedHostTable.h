//============================================================================
// Name        : PersonalityTable.h
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
// Description : A hash table of ScannedHosts and accompanying helper functions
//============================================================================

#ifndef SCANNEDHOSTTREE_H_
#define SCANNEDHOSTTREE_H_

#include "ScannedHost.h"
#include "../HashMapStructs.h"

//total num_hosts is the total num of unique hosts counted.
//total avail_addrs is the total num of ip addresses avail on the subnet

//Mapping of Personality objects using the OS name as the key
// contains:
/* map of occuring ports (Number_Protocol for key Ex: 22_TCP)
 *  - all behaviors assumed to be open if we have an entry.
 *  - determine default behaviors for TCP, UDP & ICMP somehow.
 * m_count of number of hosts w/ this OS.
 * m_port_count - number of open ports counted for hosts w/ this OS
 * map of occuring MAC addr vendors, so we know what types of NIC's are used for machines of a similar type on a network.
 */

//HashMap of Personality objects; Key is personality specific name (i.e. Linux 2.6.35-2.6.38), Value is ptr to Personality object
typedef Nova::HashMap<std::string, class Nova::ScannedHost *, std::hash<std::string>, eqstr > ScannedHost_Table;

namespace Nova
{

class ScannedHostTable
{

public:

	ScannedHostTable();

	~ScannedHostTable();

	// Adds a host to the Personality Table if it's not there; if it is, just aggregate the values
	//  Personality *add - pointer to a Personality object that contains new information for the table
	// No return value
	void AddHost(ScannedHost *add);

	//Increment every time a host is added
	unsigned long int m_num_of_hosts;

	//Used to keep track of hosts that yielded an actual personality object
	unsigned long int m_num_used_hosts;

	//Start with range of the subnets, decrement every time host is added
	unsigned long int m_numAddrsAvail;

	//HashMAP[std::string key]; key == Personality, val == ptr to Personality object
	ScannedHost_Table m_personalities;
};

}

#endif
