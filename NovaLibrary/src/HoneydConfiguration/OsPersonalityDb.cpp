//============================================================================
// Name        : VendorMacDb.cpp
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
// Description : Class for using the nmap MAC prefix file
//============================================================================

#include "OsPersonalityDb.h"
#include "../NovaUtil.h"
#include "../Config.h"
#include "../Logger.h"

#include <errno.h>
#include <fstream>
#include <math.h>
#include <sstream>
#include <string.h>
#include <algorithm>

using namespace std;
using namespace Nova;

OsPersonalityDb::OsPersonalityDb()
{
}

vector<string> OsPersonalityDb::GetPersonalityOptions() {
	vector<string> options;
	for(uint i = 0; i < m_nmapPersonalities.size(); i++)
	{
		options.push_back(m_nmapPersonalities.at(i).first);
	}

	vector<string>::iterator newEnd = unique(options.begin(), options.end());
	options.erase(newEnd, options.end());

	return options;
}

void OsPersonalityDb::LoadNmapPersonalitiesFromFile()
{
	string NMapFile = Config::Inst()->GetPathShared() + "/nmap-os-db";
	ifstream nmapPers(NMapFile.c_str());
	string line, fprint, prefix, printClass;
	if(nmapPers.is_open())
	{
		while(nmapPers.good())
		{
			getline(nmapPers, line);
			// From 'man strtoul'  Since strtoul() can legitimately return 0 or  LONG_MAX  (LLONG_MAX  for
			// strtoull()) on both success and failure, the calling program should set
			// errno to 0 before the call, and then determine if an error occurred  by
			// checking whether errno has a nonzero value after the call.

			//We've hit a fingerprint line
			prefix = "Fingerprint";
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				//Remove 'Fingerprint ' prefix.
				line = line.substr(prefix.size()+1,line.size());
				fprint = line;

				//All fingerprint lines are followed by one or more class lines, get the first class line
				getline(nmapPers, line);
				prefix = "Class";

				// TODO This could actually have more than one class line. However,
				// the nmap-os-db fingerprint line currently isn't machine parsable.
				// Mulitple device names can be split up with or, semicolon, or other thing.
				// Might fix if nmap ever makes the formatting of the Fingerprint lines better for mult devices
				if(!line.substr(0, prefix.size()).compare(prefix))
				{
					printClass = line.substr(prefix.size()+1, line.size());
					pair<string, string> printPair;
					printPair.first = fprint;
					printPair.second = printClass;
					m_nmapPersonalities.push_back(printPair);
				}
				else
				{
					LOG(ERROR, "ERROR: Unable to load nmap fingerpint: "+fprint, "");
				}
			}
		}
	}
	nmapPers.close();
}

