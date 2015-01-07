//============================================================================
// Name        : PortSet.cpp
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

#include "PortSet.h"
#include "Script.h"
#include "HoneydConfiguration.h"
#include "../Config.h"
#include "../Logger.h"

#include <sstream>
#include <fstream>

using namespace std;

namespace Nova
{

PortSet::PortSet()
{
	m_defaultTCPBehavior = PORT_CLOSED;
	m_defaultUDPBehavior = PORT_CLOSED;
	m_defaultICMPBehavior = PORT_OPEN;
}

bool PortSet::AddPort(Port port)
{
	if(port.m_protocol == PROTOCOL_TCP || port.m_protocol == PROTOCOL_UDP)
	{
		m_portExceptions.push_back(port);
	}
	else
	{
		return false;
	}
	return true;
}

bool PortSet::SetTCPBehavior(const string &behavior)
{
	if(!behavior.compare("open"))
	{
		m_defaultTCPBehavior = PORT_OPEN;
	}
	else if(!behavior.compare("closed"))
	{
		m_defaultTCPBehavior = PORT_CLOSED;
	}
	else if(!behavior.compare("filtered"))
	{
		m_defaultTCPBehavior = PORT_FILTERED;
	}
	else
	{
		return false;
	}
	return true;
}

string PortSet::ToString(const string &profileName)
{
	// This is used to name the script specific config files. Sort of hacky, they should probably be named something
	// based off the portset name and port, but that would require a bunch of filename sanitation and they're not really
	// meant to be human read anyway
	static int configFileIndex = 0;
	stringstream out;

	//Defaults
	out << "set " << profileName << " default tcp action " << Port::PortBehaviorToString(m_defaultTCPBehavior) << '\n';
	out << "set " << profileName << " default udp action " << Port::PortBehaviorToString(m_defaultUDPBehavior) << '\n';
	out << "set " << profileName << " default icmp action " << Port::PortBehaviorToString(m_defaultICMPBehavior) << '\n';

	//TCP Exceptions
	for(uint i = 0; i < m_portExceptions.size(); i++)
	{
		//If it's a script then we need to print the script path, not "script"
		if((m_portExceptions[i].m_behavior == PORT_SCRIPT) || (m_portExceptions[i].m_behavior == PORT_TARPIT_SCRIPT))
		{
			Script script = HoneydConfiguration::Inst()->GetScript(m_portExceptions[i].m_scriptName);

			if (script.m_isConfigurable)
			{

				stringstream ss;
				ss << Config::Inst()->GetPathHome() << "/config/haystackscripts/" << configFileIndex;
				configFileIndex++;
				string scriptConfigPath = ss.str();

				ofstream configFile;
				configFile.open (scriptConfigPath);

				// Put default strings for anything that isn't set
				for (map<string, vector<string>>::iterator optionIter = script.options.begin(); optionIter != script.options.end(); optionIter++)
				{
					bool found = false;
					for (map<string, string>::iterator it = m_portExceptions[i].m_scriptConfiguration.begin(); it != m_portExceptions[i].m_scriptConfiguration.end(); it++)
					{
						if (it->first == optionIter->first)
						{
							found = true;
							break;
						}
					}

					if (!found)
					{
						if (script.options[optionIter->first].size() > 0)
						{
							//LOG(DEBUG, "Writing default value because no user specified option for key " + optionIter->first, "");
							configFile << optionIter->first << " " << script.options[optionIter->first].at(0) << endl;
						}
						else
						{
							LOG(ERROR, "Script contained no value options for the key " + optionIter->first, "");
						}
					}
				}

				// Put the values for any strings the user set
				for (map<string, string>::iterator it = m_portExceptions[i].m_scriptConfiguration.begin(); it != m_portExceptions[i].m_scriptConfiguration.end(); it++)
				{
					configFile << it->first << " " << it->second << endl;
				}
				configFile.close();

				out << "add " << profileName << " " << Port::PortProtocolToString(m_portExceptions[i].m_protocol) << " port " << m_portExceptions[i].m_portNumber << " \"" << script.m_path << " " << scriptConfigPath << "\"\n";
			}
			else
			{
				out << "add " << profileName << " " << Port::PortProtocolToString(m_portExceptions[i].m_protocol) << " port "  << m_portExceptions[i].m_portNumber << " \"" << script.m_path << "\"\n";
			}
		}
		else
		{
			out << "add " << profileName << " " << Port::PortProtocolToString(m_portExceptions[i].m_protocol) << " port " << m_portExceptions[i].m_portNumber << " " <<
					Port::PortBehaviorToString(m_portExceptions[i].m_behavior) << "\n";
		}
	}

	return out.str();
}

}

