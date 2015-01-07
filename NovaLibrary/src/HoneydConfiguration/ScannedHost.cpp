//============================================================================
// Name        : ScannedHost.cpp
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

#include "ScannedHost.h"

using namespace std;

namespace Nova
{
ScannedHost::ScannedHost()
{
	m_count = 1;
	m_port_count = 0;
	m_osclass = "";
	m_uptime = 0;
}

ScannedHost::~ScannedHost()
{

}

void ScannedHost::AddVendor(const string &vendor)
{
	int index = -1;
	for(uint i = 0; i < m_vendors.size(); i++)
	{
		if(vendor == m_vendors[i].first)
		{
			index = i;
			break;
		}
	}

	if(index == -1)
	{
		m_vendors.push_back(pair<string, uint>(vendor, 1));
	}
	else
	{
		m_vendors[index].second++;
	}
}

void ScannedHost::AddVendor(const string &vendor, uint count)
{
	if(count == 0)
	{
		return;
	}

	int index = -1;
	for(uint i = 0; i < m_vendors.size(); i++)
	{
		if(vendor == m_vendors[i].first)
		{
			index = i;
			break;
		}
	}

	if(index == -1)
	{
		m_vendors.push_back(pair<string, uint>(vendor, count));
	}
	else
	{
		m_vendors[index].second += count;
	}
}

}
