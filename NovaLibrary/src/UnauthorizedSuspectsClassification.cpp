//============================================================================
// Name        : UnauthorizedSuspectsClassification.cpp
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

#include "UnauthorizedSuspectsClassification.h"
#include "Config.h"
#include "Logger.h"

using namespace std;

namespace Nova
{


UnauthorizedSuspectsClassification::UnauthorizedSuspectsClassification() {
	string filePath = Config::Inst()->GetPathHome() + "/config/authorizedIPs.config";

	ifstream configFile(filePath);
	string line;

	while (configFile.good())
	{
		getline(configFile, line);

		// Skip lines too short to be a valid IP address
		if (line.length() < 7)
		{
			continue;
		}

		// Skip comment lines
		if (line.at(0) == '#')
		{
			continue;
		}

		// Check for valid IP address
		struct in_addr ip;
		if (inet_pton(AF_INET, line.c_str(), &ip) == 0)
		{
			LOG(ERROR, "Authorized IP file had invalid IP address: " + line, "");
			continue;
		}

		m_authorizedIPs.push_back(line);
	}

	configFile.close();
}

UnauthorizedSuspectsClassification::~UnauthorizedSuspectsClassification() {}
void UnauthorizedSuspectsClassification::LoadConfiguration(string) {}



// Assume everyone is hostile unless they're in authorized list
double UnauthorizedSuspectsClassification::Classify(Suspect *suspect)
{
	double classification = 0;

	if (find(m_authorizedIPs.begin(), m_authorizedIPs.end(), suspect->GetIpString()) == m_authorizedIPs.end())
	{
		suspect->m_classificationNotes += "\n=== Notes from Unauthorized Suspect Engine ===\n";
		suspect->m_classificationNotes += "Suspect is not in the list of authorized network IP addresses!";
		classification = 1;
	}

	return classification;
}

}
