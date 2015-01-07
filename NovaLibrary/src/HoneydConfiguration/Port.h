//============================================================================
// Name        : Port.h
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
// Description : Represents a single instance of a TCP or UDP port that has been
//	scanned and identified on a host by Nmap. Small enough of a class such that
//	making copies is acceptable
//============================================================================

#ifndef PORT_H_
#define PORT_H_

#include <string>
#include <map>
#include "../HashMap.h"
#include "../HashMapStructs.h"



namespace Nova
{


enum PortBehavior
{
	PORT_FILTERED = 0
	, PORT_CLOSED
	, PORT_OPEN
	, PORT_SCRIPT
	, PORT_TARPIT_OPEN
	, PORT_TARPIT_SCRIPT
	, PORT_ERROR
};

enum PortProtocol
{
	PROTOCOL_UDP = 0
	, PROTOCOL_TCP
	, PROTOCOL_ICMP
	, PROTOCOL_ERROR
};

class Port
{

public:

	Port(std::string serviceName, enum PortProtocol protocol, uint portNumber, enum PortBehavior behavior);
	Port(){};

	//Returns a string representation of the given PortProtocol
	//	Returns one of: "udp" "tcp" "icmp" or "" for error
	static std::string PortProtocolToString(enum PortProtocol protocol);

	//Returns a PortProtocol enum which is represented by the given string
	//	returns PROTOCOL_ERROR on error
	static enum PortProtocol StringToPortProtocol(std::string protocol);

	//Returns a string representation of the given PortBehavior
	//	Returns one of: "closed" "open" "filtered" "script" "tarpit-open" "tarpit-script" or "" for error
	static std::string PortBehaviorToString(enum PortBehavior behavior);

	//Returns a PortBehavior enum which is represented by the given string
	//	returns PORT_ERROR on error
	static enum PortBehavior StringToPortBehavior(std::string behavior);

	//Javascript compatibiltiy functions
	uint GetPortNum() {return m_portNumber;}
	std::string GetProtocol() { return PortProtocolToString(m_protocol);}
	std::string GetBehavior() { return PortBehaviorToString(m_behavior);}
	std::string GetScriptName() { return m_scriptName;}
	std::string GetService() { return m_service;}
	std::map<std::string, std::string> GetScriptConfiguration() {return m_scriptConfiguration;}
	std::string m_service;

	enum PortProtocol m_protocol;

	uint m_portNumber;

	enum PortBehavior m_behavior;

	std::string m_scriptName;

	std::map<std::string, std::string> m_scriptConfiguration;
};

}

#endif /* PORT_H_ */
