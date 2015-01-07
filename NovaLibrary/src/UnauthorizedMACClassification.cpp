//============================================================================
// Name        : UnauthorizedMACClassification.cpp
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
// Description : Child class of ClassificationEngine, specifies a list of allowed
//   IP addresses on the network, and all other IPs are marked as hostile.
//============================================================================

#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <boost/algorithm/string.hpp>

#include "UnauthorizedMACClassification.h"
#include "Config.h"
#include "Logger.h"

using namespace std;

namespace Nova
{


UnauthorizedMACClassification::UnauthorizedMACClassification() {
	string filePath = Config::Inst()->GetPathHome() + "/config/authorizedMACs.config";

	ifstream configFile(filePath);
	string line;

	while (configFile.good())
	{
		getline(configFile, line);

		if (line.length() == 0)
		{
			continue;
		}

		// Skip comment lines
		if (line.at(0) == '#')
		{
			continue;
		}

		boost::trim(line);
		boost::to_upper(line);

		if (line.length() != 17)
		{
			LOG(ERROR, "Authorized MAC file had invalid MAC address: " + line, "");
			continue;
		}

		m_authorizedMACs.push_back(line);
	}

	configFile.close();
}

UnauthorizedMACClassification::~UnauthorizedMACClassification() {}
void UnauthorizedMACClassification::LoadConfiguration(string) {}



// Assume everyone is hostile unless they're in authorized list
double UnauthorizedMACClassification::Classify(Suspect *suspect)
{
	double classification = 0;

	if (find(m_authorizedMACs.begin(), m_authorizedMACs.end(), suspect->GetMACString()) == m_authorizedMACs.end())
	{
		suspect->m_classificationNotes += "\n=== Notes from Unauthorized MAC Engine ===\n";
		suspect->m_classificationNotes += "Suspect is not in the list of authorized network MAC addresses!";
		classification = 1;
	}

	return classification;
}

}
