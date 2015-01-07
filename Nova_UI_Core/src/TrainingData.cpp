//============================================================================
// Name        : TrainingData.cpp
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

#include <fstream>
#include <sstream>
#include <ANN/ANN.h>

#include "TrainingData.h"
#include "Logger.h"

using namespace std;
using namespace Nova;

trainingDumpMap *TrainingData::ParseEngineCaptureFile(string captureFile)
{
	trainingDumpMap *trainingTable = new trainingDumpMap();

	ifstream dataFile(captureFile.data());
	string line, ip, data;

	if(dataFile.is_open())
	{
		while(dataFile.good() && getline(dataFile,line))
		{
			uint firstDelim = line.find_first_of(' ');

			if(firstDelim == string::npos)
			{
				LOG(ERROR, "Invalid or corrupt CE capture file.", "");
				return NULL;
			}

			ip = line.substr(0,firstDelim);
			data = line.substr(firstDelim + 1, string::npos);
			data = "\t" + data;

			if((*trainingTable)[ip] == NULL)
			{
				(*trainingTable)[ip] = new vector<string>();
			}

			(*trainingTable)[ip]->push_back(data);
		}
	}
	else
	{
		LOG(ERROR, "Unable to open CE capture file for reading.", "");
		return NULL;
	}
	dataFile.close();

	return trainingTable;
}

trainingSuspectMap *TrainingData::ParseTrainingDb(string dbPath)
{
	trainingSuspectMap *suspects = new trainingSuspectMap();

	string line;
	bool getHeader = true;
	uint delimIndex;
	uint lastUID = 0;

	trainingSuspect *suspect = new trainingSuspect();
	suspect->points = new vector<string>();

	ifstream stream(dbPath.data());
	if(stream.is_open())
	{
		while(stream.good() && getline(stream,line))
		{
			if(line.length() > 0)
			{
				if(getHeader)
				{
					delimIndex = line.find_first_of(' ');

					if(delimIndex == string::npos)
					{
						LOG(ERROR, "Invalid or corrupt DB training file.", "");
						return NULL;
					}

					stringstream ss;
					ss << lastUID;
					suspect->uid = ss.str();
					lastUID++;

					suspect->isHostile = atoi(line.substr(0, 1).c_str());

					// TODO: It would be nice to have the suspects used last time to generate the
					// data file still selected as included. For now we just mark them all.
					suspect->isIncluded = true;

					delimIndex = line.find_first_of('"');
					if(delimIndex == string::npos)
					{
						LOG(ERROR, "Invalid or corrupt DB training file.", "");
						return NULL;
					}

					suspect->description = line.substr(line.find_first_of('"'), line.length());
					getHeader = false;
				}
				else
				{
					suspect->points->push_back(line);
				}
			}
			else
			{
				if(!getHeader)
				{
					(*suspects)[suspect->uid] = suspect;
					suspect = new trainingSuspect();
					suspect->points = new vector<string>();
					getHeader = true;
				}
			}
		}
	}
	else
	{
		LOG(ERROR, "Unable to open training DB file for reading.", "");
		return NULL;
	}
	stream.close();

	return suspects;
}

bool TrainingData::CaptureToTrainingDb(string dbFile, trainingSuspectMap *entries)
{
	trainingSuspectMap *db = ParseTrainingDb(dbFile);

	if(db == NULL)
		return false;

	int max = 0, uid = 0;
	for(trainingSuspectMap::iterator it = db->begin(); it != db->end(); it++)
	{
		uid = atoi(it->first.c_str());

		if(uid > max)
			max = uid;
	}

	uid = max + 1;
	ofstream out(dbFile.data(), ios::app);
	for(trainingSuspectMap::iterator header = entries->begin(); header != entries->end(); header++)
	{
		if(header->second->isIncluded)
		{
			out << uid << " " << header->second->isHostile << " \"" << header->second->description << "\"" << endl;
			for(vector<string>::iterator i = header->second->points->begin(); i != header->second->points->end(); i++)
			{
				out << *i << endl;
			}
			out << endl;

			uid++;
		}
	}

	out.close();

	return true;
}

string TrainingData::MakaDataFile(trainingSuspectMap& db)
{
	stringstream ss;

	for(trainingSuspectMap::iterator it = db.begin(); it != db.end(); it++)
	{
		if(it->second->isIncluded)
		{
			for(uint i = 0; i < it->second->points->size(); i++)
			{
				ss << it->second->points->at(i).substr(1, string::npos) << it->second->isHostile << endl;
			}
		}
	}

	return ss.str();
}

void TrainingData::ThinTrainingPoints(trainingDumpMap *suspects, double distanceThreshhold)
{
	uint numThinned = 0, numTotal = 0;
	double maxValues[DIM];
	for(uint i = 0; i < DIM; i++)
	{
		maxValues[i] = 0;
	}

	// Parse out the max values for normalization
	for(trainingDumpMap::iterator it = suspects->begin(); it != suspects->end(); it++)
	{
		for(int p = it->second->size() - 1; p >= 0; p--)
		{
			numTotal++;
			stringstream ss(it->second->at(p));
			for(uint d = 0; d < DIM; d++)
			{
				string featureString;
				double feature;
				getline(ss, featureString, ' ');

				feature = atof(featureString.c_str());
				if(feature > maxValues[d])
				{
					maxValues[d] = feature;
				}
			}
		}
	}


	ANNpoint newerPoint = annAllocPt(DIM);
	ANNpoint olderPoint = annAllocPt(DIM);

	for(trainingDumpMap::iterator it = suspects->begin(); it != suspects->end(); it++)
	{
		// Can't trim points if there's only 1
		if(it->second->size() < 2)
		{
			continue;
		}

		stringstream ss(it->second->at(it->second->size() - 1));
		for(int d = 0; d < DIM; d++)
		{
			string feature;
			getline(ss, feature, ' ');
			newerPoint[d] = atof(feature.c_str()) / maxValues[d];
		}

		for(int p = it->second->size() - 2; p >= 0; p--)
		{
			double distance = 0;

			stringstream ss(it->second->at(p));
			for(uint d = 0; d < DIM; d++)
			{
				string feature;
				getline(ss, feature, ' ');
				olderPoint[d] = atof(feature.c_str()) / maxValues[d];
			}

			for(uint d=0; d < DIM; d++)
			{
				distance += annDist(d, olderPoint,newerPoint);
			}

			// Should we throw this point away?
			if(distance < distanceThreshhold)
			{
				it->second->erase(it->second->begin() + p);
				numThinned++;
			}
			else
			{
				for(uint d = 0; d < DIM; d++)
				{
					newerPoint[d] = olderPoint[d];
				}
			}
		}
	}
	stringstream ss;
	ss << " Total points: "  << numTotal;
	LOG(INFO, ss.str(), "");
	ss.str("");
	ss << " Number Thinned:" << numThinned;
	LOG(INFO, ss.str(), "");

	annDeallocPt(newerPoint);
	annDeallocPt(olderPoint);
}
