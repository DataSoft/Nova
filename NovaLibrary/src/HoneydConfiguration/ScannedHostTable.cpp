//============================================================================
// Name        : PersonalityTable.cpp
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

#include "ScannedHostTable.h"
#include "../Logger.h"

#include <sstream>

using namespace std;

namespace Nova
{

ScannedHostTable::ScannedHostTable()
{
	m_num_of_hosts = 0;
}

ScannedHostTable::~ScannedHostTable()
{

}

// Add a single Host
void ScannedHostTable::AddHost(ScannedHost *add)
{
	// If the personality pointer that we're passing to the function
	// doesn't exist in the personalities table, just create a new
	// instance using the bracket operator and assign the Value to
	// the pointer we're passing.
	if(m_personalities.find(add->m_personalityClass[0]) == m_personalities.end())
	{
		m_personalities[add->m_personalityClass[0]] = add;
	}
	// If the personality does exist, however, then
	// we need to do some extra work to aggregate its attributes
	// with the existing entry.
	else
	{
		// Create a copy of the pointer that exists within the table
		// to modify.
		ScannedHost *currentHost = m_personalities[add->m_personalityClass[0]];
		// Since we're adding a host, just grab the single IP and MAC
		// address that exist inside of the Personality object and
		// increment the number of times a host has had this personality
		// by one.
		if(!add->m_macs.empty())
		{
			currentHost->m_macs.push_back(add->m_macs[0]);
		}
		currentHost->m_addresses.push_back(add->m_addresses[0]);
		currentHost->m_count++;

		//Add the PortSets of the added into the current one
		for(uint i = 0; i < add->m_portSets.size(); i ++)
		{
			currentHost->m_portSets.push_back(add->m_portSets[i]);
		}

		for(uint i = 0; i < add->m_vendors.size(); i++)
		{
			currentHost->AddVendor(add->m_vendors[i].first, add->m_vendors[i].second);
		}

		// Finally, delete the pointer we passed to deallocate
		// its memory.
		delete add;
	}
}

}
