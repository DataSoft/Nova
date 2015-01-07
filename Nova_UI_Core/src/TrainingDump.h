//============================================================================
// Name        : TrainingData.h
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
// Description : Controls suspect classification data
//============================================================================

#ifndef TRAININGDUMP_H_
#define TRAININGDUMP_H_

#include <string>
#include <vector>

#include "HashMapStructs.h"

namespace Nova {

// Header for training data
struct _trainingFileSuspect
{
	bool isHostile;
	bool isIncluded;
	std::string uid;
	std::string description;
	std::vector<std::string> points;
};

typedef struct _trainingFileSuspect trainingFileSuspect;

typedef Nova::HashMap<std::string, trainingFileSuspect*, std::hash<std::string>, eqstr > trainingFileSuspectMap;

class TrainingDump
{
public:
	// Parse a CE dump file
	TrainingDump();

	bool LoadCaptureFile(std::string pathDumpFile);

	bool SetDescription(std::string uid, std::string description);
	bool SetIsHostile(std::string uid, bool isHostile);
	bool SetIsIncluded(std::string uid, bool isIncluded);

	bool SetAllIsIncluded(bool isIncluded);
	bool SetAllIsHostile(bool isHostile);

	bool SaveToDb(std::string dbFile);

	bool MergeIPs(std::vector<std::string> idsToMerge, std::string newName);
	bool MergeBenign(std::string newName);

	// Removes consecutive points who's squared distance is less than a specified distance
	void ThinTrainingPoints(double distanceThreshhold);

private:
	trainingFileSuspectMap *trainingTable;

};

}

#endif /* TRAININGDUMP_H_ */
