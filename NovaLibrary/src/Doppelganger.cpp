//============================================================================
// Name        : Doppelganger.cpp
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
// Description : Set of functions used by Novad for masking local host information
//============================================================================

#include "Config.h"
#include "Logger.h"
#include "NovaUtil.h"
#include "Doppelganger.h"
#include "Database.h"

#include <string>
#include <sstream>
#include <sys/types.h>
#include <arpa/inet.h>

using namespace std;

namespace Nova
{

// suspects: Uses the hostile suspects in this suspect table to determine Dopp routing
Doppelganger::Doppelganger(DatabaseQueue& suspects)
: m_suspectTable(suspects)
{
	m_initialized = false;
}

Doppelganger::~Doppelganger()
{
}

//Synchronizes an initialized Doppelganger object with it's suspect table
// *Note if the Dopp was never initialized this function initializes it.
void Doppelganger::UpdateDoppelganger()
{
	if(!Config::Inst()->GetIsDmEnabled())
	{
		return;
	}

	if(!m_initialized)
	{
		InitDoppelganger();
	}

	Database::Inst()->StartTransaction();
	vector<SuspectID_pb> keys = Database::Inst()->GetHostileSuspects();
	Database::Inst()->StopTransaction();

	vector<SuspectID_pb> keysCopy = keys;

	//A few variable declarations
	SuspectID_pb temp;
	uint i = 0;
	bool found = false;

	string prefix = "sudo iptables -t nat -I DOPP -s ";
	string suffix = " -j DNAT --to-destination "+Config::Inst()->GetDoppelIp();

	stringstream ss;
	in_addr inAddr;

	//Until we've finished checking each hostile suspect
	while(!keys.empty())
	{
		//Get a suspect
		temp = keys.back();
		keys.pop_back();
		found = false;

		//Look for the suspect
		for(i = 0; i < m_suspectKeys.size(); i++)
		{
			//If we find the suspect
			if((m_suspectKeys[i].m_ip() == temp.m_ip()) && (m_suspectKeys[i].m_ifname() == temp.m_ifname()))
			{
				found = true;
				//Erase matched suspect from previous suspect list, this tells us if any suspects were removed
				m_suspectKeys.erase(m_suspectKeys.begin() + i);
				break;
			}
		}

		//If the suspect is new
		if(!found)
		{
			ss.str("");
			inAddr.s_addr = htonl((in_addr_t)temp.m_ip());
			ss << prefix << inet_ntoa(inAddr) << suffix;

			LOG(DEBUG, "Doppelganger running command: " + ss.str(), "");
			if(system(ss.str().c_str()) != 0)
			{
				LOG(ERROR, "Error routing suspect to Doppelganger", "Command '"+ss.str()+"' was unsuccessful.");
			}
		}
	}
	prefix = "sudo iptables -t nat -D DOPP -s ";

	//If any suspects remain in m_suspectKeys then they need to be removed from the rule chain
	while(m_suspectKeys.size())
	{
		temp = m_suspectKeys.back();
		m_suspectKeys.pop_back();

		ss.str("");
		inAddr.s_addr = htonl((in_addr_t)temp.m_ip());
		ss << prefix << inet_ntoa(inAddr) << suffix;

		if(system(ss.str().c_str()) != 0)
		{
			LOG(ERROR, "Error removing suspect from Doppelganger list.", "Command '"+ss.str()+"' was unsuccessful.");
		}
	}
	m_suspectKeys = keysCopy;
}

//Clears the routing rules, this disables the doppelganger until init is called again.
void Doppelganger::ClearDoppelganger()
{
	if(!Config::Inst()->GetIsDmEnabled())
	{
		return;
	}

	string commandLine, prefix = "sudo iptables -F";

	commandLine = prefix + "-D FORWARD -i "+  Config::Inst()->GetDoppelInterface() + " -j DROP";
	if(system(commandLine.c_str()) != 0)
	{
		LOG(DEBUG, "Unable to remove Doppelganger rule, does it exist?", "Command '"+commandLine+"' was unsuccessful.");
	}

	prefix = "sudo iptables -t nat ";
	commandLine = prefix + "-F";
	if(system(commandLine.c_str()) != 0)
	{
		LOG(DEBUG, "Unable to remove Doppelganger rule, does it exist?", "Command '"+commandLine+"' was unsuccessful.");
	}
	vector<string> ifList = Config::Inst()->GetInterfaces();
	while(!ifList.empty())
	{
		string hostIP = GetLocalIP(ifList.back().c_str());
		commandLine = prefix + "-D PREROUTING -d "+ hostIP + " -j DOPP";
		if(system(commandLine.c_str()) != 0)
		{
			LOG(DEBUG, "Unable to remove Doppelganger rule, does it exist?", "Command '"+commandLine+"' was unsuccessful.");
		}
		ifList.pop_back();
	}

	commandLine = prefix + "-X DOPP";
	if(system(commandLine.c_str()) != 0)
	{
		LOG(DEBUG, "Unable to remove Doppelganger rule, does it exist?", "Command '"+commandLine+"' was unsuccessful.");
	}

	commandLine = "sudo route del "+Config::Inst()->GetDoppelIp();
	if(system(commandLine.c_str()) != 0)
	{
		LOG(DEBUG, "Unable to remove Doppelganger host, does it exist?", "Command '"+commandLine+"' was unsuccessful.");
	}
	m_initialized = false;
}

//Initializes the base routing rules the Doppelganger needs to operate.
// Note: This function will simply return without executing if the Doppelganger has
// called InitDoppelganger since construction or the last ClearDoppelganger();
void Doppelganger::InitDoppelganger()
{
	if(!Config::Inst()->GetIsDmEnabled())
	{
		return;
	}

	if(m_initialized)
	{
		return;
	}
	string commandLine = "sudo iptables -A FORWARD -i "+Config::Inst()->GetDoppelInterface()+" -j DROP";

	if(system(commandLine.c_str()) != 0)
	{
		LOG(ERROR, "Error setting up system for Doppelganger", "Command '"+commandLine+"' was unsuccessful.");
	}

	commandLine = "sudo route add -host "+Config::Inst()->GetDoppelIp()+" dev "+Config::Inst()->GetDoppelInterface();

	if(system(commandLine.c_str()) != 0)
	{
		LOG(ERROR, "Error setting up system for Doppelganger", "Command '"+commandLine+"' was unsuccessful.");
	}

	commandLine = "sudo iptables -t nat -N DOPP";
	if(system(commandLine.c_str()) != 0)
	{
		/*LOG(NOTICE, "Error setting up system for Doppelganger", "Command '"+commandLine+"' was unsuccessful."
			" Attempting to flush 'DOPP' rule chain if it already exists.");*/
		commandLine = "sudo iptables -t nat -F DOPP";
		if(system(commandLine.c_str()) != 0)
		{
			LOG(ERROR, "Error setting up system for Doppelganger", "Command '"+commandLine+"' was unsuccessful."
				" Unable to flush or create 'DOPP' rule-chain");
		}
	}
	vector<string> ifList = Config::Inst()->GetInterfaces();
	while(!ifList.empty())
	{
		if(!ifList.back().size())
		{
			ifList.pop_back();
			continue;
		}
		string hostIP = GetLocalIP(ifList.back().c_str());
		commandLine = "sudo iptables -t nat -I PREROUTING -d "+hostIP+" -j DOPP";
		if(system(commandLine.c_str()) != 0)
		{
			LOG(ERROR, "Error setting up system for Doppelganger", "Command '"+commandLine+"' was unsuccessful.");
		}
		ifList.pop_back();
	}
	m_initialized = true;
}

//Clears and Initializes the Doppelganger then updates the routing list from scratch.
void Doppelganger::ResetDoppelganger()
{
	if(!Config::Inst()->GetIsDmEnabled())
	{
		return;
	}

	ClearDoppelganger();
	InitDoppelganger();

	string buf, commandLine, prefix = "sudo ipables -t nat ";

	commandLine = prefix + "-F DOPP";
	if(system(commandLine.c_str()) != 0)
	{
		LOG(DEBUG, "Unable to flush Doppelganger rules.", "Command '"+commandLine+"' was unsuccessful.");
	}
	m_suspectKeys.clear();

	Database::Inst()->StartTransaction();
	m_suspectKeys = Database::Inst()->GetHostileSuspects();
	Database::Inst()->StopTransaction();

	prefix = "sudo iptables -t nat -I DOPP -s ";
	string suffix = " -j DNAT --to-destination " + Config::Inst()->GetDoppelIp();

	stringstream ss;
	in_addr inAddr;

	for(uint i = 0; i < m_suspectKeys.size(); i++)
	{
		ss.str("");
		inAddr.s_addr = (in_addr_t)m_suspectKeys[i].m_ip();
		ss << prefix << inet_ntoa(inAddr) << suffix;
		if(system(ss.str().c_str()) != 0)
		{
			LOG(ERROR, "Error routing suspect to Doppelganger", "Command '"+commandLine+"' was unsuccessful.");
		}
	}
}

}
