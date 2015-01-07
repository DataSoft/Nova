//============================================================================
// Name        : PortSet.h
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
// Description : Represents a collection of ports for a ScannedHost and their
//		respective observed behaviors. ICMP is included as a "port" for the
//		purposes of this structure
//============================================================================

#ifndef PORTSET_H_
#define PORTSET_H_

#include "Port.h"

#include <vector>

namespace Nova
{

class PortSet
{

public:

	PortSet();

	//Add a port into the set.
	bool AddPort(Port port);

	//Set the TCP default behavior through a string
	//	behavior - One of "open" "closed" or "filtered"
	//	returns - True on success, false on error
	bool SetTCPBehavior(const std::string &behavior);

	//Returns a string representation of the PortSet suitable for insertion into the Honeyd Config file
	std::string ToString(const std::string &profileName);

	//Javascript compatibility functions
	std::string GetTCPBehavior() {return Port::PortBehaviorToString(m_defaultTCPBehavior);}
	std::string GetUDPBehavior() {return Port::PortBehaviorToString(m_defaultUDPBehavior);}
	std::string GetICMPBehavior() {return Port::PortBehaviorToString(m_defaultICMPBehavior);}

	enum PortBehavior m_defaultTCPBehavior;
	enum PortBehavior m_defaultUDPBehavior;

	std::vector <Port> m_portExceptions;

	//ICMP set
	enum PortBehavior m_defaultICMPBehavior;

};

}

#endif /* PORTSET_H_ */
