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
// Description :
//============================================================================

// !!!WARNING!!!
// This interface has been deprecated. Use TrainingDump.cpp instead whenever possible.
// !!!WARNING!!!

#ifndef TRAININGDATA_H_
#define TRAININGDATA_H_

#include <string>
#include <vector>

#include "HashMapStructs.h"

namespace Nova {

// Header for training data
struct _trainingSuspect
{
	bool isHostile;
	bool isIncluded;
	std::string uid;
	std::string description;
	std::vector<std::string> *points;
};

typedef struct _trainingSuspect trainingSuspect;

typedef HashMap<std::string, trainingSuspect*, std::hash<std::string>, eqstr > trainingSuspectMap;
typedef HashMap<std::string, std::vector<std::string>*, std::hash<std::string>, eqstr > trainingDumpMap;

class TrainingData
{
public:
	// Convert CE dump to Training DB format and append it
	static bool CaptureToTrainingDb(std::string dbFile, trainingSuspectMap *selectionOptions);

	// Parse a CE dump file
	static trainingDumpMap *ParseEngineCaptureFile(std::string captureFile);

	// Parse a Training DB file
	static trainingSuspectMap *ParseTrainingDb(std::string dbPath);

	// Create a CE data file from a subset of the Training DB file
	static std::string MakaDataFile(trainingSuspectMap& db);

	// Removes consecutive points who's squared distance is less than a specified distance
	static void ThinTrainingPoints(trainingDumpMap *suspects, double distanceThreshhold);

};

}

#endif /* TRAININGDATA_H_ */
