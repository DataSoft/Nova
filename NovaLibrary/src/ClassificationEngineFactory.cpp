//============================================================================
// Name        : ClassificationEngineFactory.cpp
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
// Description :
//============================================================================

#include "UnauthorizedSuspectsClassification.h"
#include "ThresholdTriggerClassification.h"
#include "UnauthorizedMACClassification.h"
#include "ClassificationEngineFactory.h"
#include "ScriptAlertClassification.h"
#include "KnnClassification.h"
#include "Logger.h"

using namespace std;
using namespace Nova;

// Factory method for classification engine creation
ClassificationEngine * MakeEngine(std::string engine)
{
	if (engine == "KNN")
	{
		return new KnnClassification();
	}
	else if (engine == "THRESHOLD_TRIGGER")
	{
		return new ThresholdTriggerClassification();
	}
	else if (engine == "SCRIPT_ALERT")
	{
		return new ScriptAlertClassification();
	}
	else if (engine == "UNAUTHORIZED_SUSPECTS")
	{
		return new UnauthorizedSuspectsClassification();
	}
	else if (engine == "UNAUTHORIZED_MACS")
	{
		return new UnauthorizedMACClassification();
	}
	else
	{
		LOG(ERROR, "Unknown classification engine type: " + engine + ". Known types are KNN, THRESHOLD_TRIGGER, and SCRIPT_ALERT", "");
	}

	return NULL;
}
