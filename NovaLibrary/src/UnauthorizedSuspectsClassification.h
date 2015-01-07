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

#ifndef UNAUTHORIZEDSUSPECTSCLASSIFICATION_H_
#define UNAUTHORIZEDSUSPECTSCLASSIFICATION_H_

#include <string>
#include <vector>

#include "ClassificationEngine.h"
#include "Suspect.h"

namespace Nova
{

class UnauthorizedSuspectsClassification: public Nova::ClassificationEngine {
public:
	UnauthorizedSuspectsClassification();
	virtual ~UnauthorizedSuspectsClassification();

	void LoadConfiguration(std::string);
	double Classify(Suspect *suspect);

private:
	std::vector<std::string> m_authorizedIPs;

};

}

#endif /* UNAUTHORIZEDSUSPECTSCLASSIFICATION_H_ */
