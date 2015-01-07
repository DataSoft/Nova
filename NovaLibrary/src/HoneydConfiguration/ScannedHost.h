//============================================================================
// Name        : ScannedHost.h
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
// Description : Represents one or more hosts that have been scanned by Nmap and
//	identified as the same OS.
//============================================================================

#ifndef SCANNEDHOST_H_
#define SCANNEDHOST_H_

#include "PortSet.h"

namespace Nova
{

class ScannedHost
{

public:

	ScannedHost();
	~ScannedHost();

	// Adds a vendor to the m_vendors member variable HashMap
	//  std::string vendor - the string representation of the vendor taken from the nmap xml
	// Returns nothing; if the vendor is there, it increments the count, if it isn't it adds it
	// and sets count to one.
	void AddVendor(const std::string &);
	void AddVendor(const std::string &, uint count);

	// count of the number of instances of this host on the scanned subnets
	uint m_count;

	// total number of ports found for this host on the scanned subnets
	uint m_port_count;

	//uptime in seconds
	uint m_uptime;

	// pushed in this order: name, type, osgen, osfamily, vendor
	// pops in this order: vendor, osfamily, osgen, type, name
	std::vector<std::string> m_personalityClass;
	std::string m_osclass;

	std::string m_personality;

	// vector of MAC addresses
	std::vector<std::string> m_macs;

	// vector of IP addresses
	std::vector<std::string> m_addresses;

	std::vector<std::pair<std::string, uint> > m_vendors;

	//A collection of PortSets, representing each group of ports found
	std::vector<PortSet *> m_portSets;

};

}

#endif
