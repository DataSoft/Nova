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
// Description : Controls suspect classification data
//============================================================================

#include <fstream>
#include <sstream>
#include <ANN/ANN.h>

#include "TrainingDump.h"
#include "Logger.h"

using namespace std;
using namespace Nova;

TrainingDump::TrainingDump()
{
	trainingTable = NULL;
}

bool TrainingDump::LoadCaptureFile(string pathDumpFile)
{
	if(trainingTable != NULL)
	{
		for(trainingFileSuspectMap::iterator it = trainingTable->begin(); it != trainingTable->end(); it++)
		{
			delete it->second;
		}
	}
	delete trainingTable;
	trainingTable = NULL;

	trainingTable = new trainingFileSuspectMap();

	ifstream dataFile(pathDumpFile.data());
	string line, ip, data;

	if(dataFile.is_open())
	{
		while(dataFile.good() && getline(dataFile,line))
		{
			uint firstDelim = line.find_first_of(' ');

			if(firstDelim == string::npos)
			{
				LOG(ERROR, "Invalid or corrupt CE capture file.", "");
				return false;
			}

			ip = line.substr(0,firstDelim);
			data = line.substr(firstDelim + 1, string::npos);
			data = "\t" + data;

			if((*trainingTable)[ip] == NULL)
			{
				(*trainingTable)[ip] = new trainingFileSuspect;
				(*trainingTable)[ip]->description = "-";
			}

			(*trainingTable)[ip]->points.push_back(data);
		}
	}
	else
	{
		LOG(ERROR, "Unable to open CE capture file for reading.", "");
		return false;
	}
	dataFile.close();
	return true;
}

bool TrainingDump::SetDescription(string uid, string description)
{
	if(!trainingTable->keyExists(uid))
	{
		return false;
	}

	(*trainingTable)[uid]->description = description;
	return true;
}

bool TrainingDump::SetIsHostile(string uid, bool isHostile)
{
	if(!trainingTable->keyExists(uid))
	{
		return false;
	}

	(*trainingTable)[uid]->isHostile = isHostile;
	return true;
}

bool TrainingDump::SetIsIncluded(string uid, bool isIncluded)
{
	if(!trainingTable->keyExists(uid))
	{
		return false;
	}


	(*trainingTable)[uid]->isIncluded = isIncluded;
	return true;
}

bool TrainingDump::SetAllIsIncluded(bool isIncluded)
{
	for(trainingFileSuspectMap::iterator it = trainingTable->begin(); it != trainingTable->end(); it++)
	{
		it->second->isIncluded = isIncluded;
	}
	return true;
}


bool TrainingDump::SetAllIsHostile(bool isHostile)
{
	for(trainingFileSuspectMap::iterator it = trainingTable->begin(); it != trainingTable->end(); it++)
	{
		it->second->isHostile = isHostile;
	}
	return true;
}

bool TrainingDump::SaveToDb(string dbFile)
{
	ThinTrainingPoints(Config::Inst()->GetThinningDistance());

	ofstream out(dbFile.data(), ios::app);
	for(trainingFileSuspectMap::iterator header = trainingTable->begin(); header != trainingTable->end(); header++)
	{
		if(header->second->isIncluded)
		{
			out << header->second->isHostile << " \"" << header->second->description << "\"" << endl;
			for(vector<string>::iterator i = header->second->points.begin(); i != header->second->points.end(); i++)
			{
				out << *i << endl;
			}
			out << endl;
		}
	}

	out.close();
	return true;
}

bool TrainingDump::MergeIPs(vector<string> idsToMerge, string newName)
{
    string rootuid = newName;
    (*trainingTable)[rootuid] = new _trainingFileSuspect();
    (*trainingTable)[rootuid]->points = vector<string>();
    (*trainingTable)[rootuid]->isIncluded = true;
    (*trainingTable)[rootuid]->isHostile = true;
    (*trainingTable)[rootuid]->uid = rootuid;
    (*trainingTable)[rootuid]->description = rootuid;

    for(uint i = 0; i < idsToMerge.size(); i++)
    {
        string id = idsToMerge.at(i);

        if (!trainingTable->keyExists(id))
        {
        	continue;
        }

        // Append all the old suspect's points to our new group entry
        for(uint j = 0; j < (*trainingTable)[id]->points.size(); j++)
        {
            (*trainingTable)[rootuid]->points.push_back((*trainingTable)[id]->points.at(j));
        }

        delete trainingTable->get(id);
        trainingTable->erase(id);
    }

    return true;
}

bool TrainingDump::MergeBenign(std::string newName)
{
	vector<string> benignIps;
	for (trainingFileSuspectMap::iterator it = trainingTable->begin(); it != trainingTable->end(); it++)
	{
		if (!it->second->isHostile)
		{
			benignIps.push_back(it->first);
		}
	}

	if (benignIps.size() <= 0)
	{
		return true;
	}

	bool error = false;
	if (!MergeIPs(benignIps, newName))
	{
		error = true;
		return error;
	}

	if (!SetIsHostile(newName, false))
	{
		error = true;
		LOG(ERROR, "Unable to set new Merged Benign group to benighn!", "");
	}

	return error;
}

// TODO: Fix this so it works with correct normalization
void TrainingDump::ThinTrainingPoints(double distanceThreshhold)
{
	uint numThinned = 0, numTotal = 0;
	double maxValues[DIM];
	for(uint i = 0; i < DIM; i++)
		maxValues[i] = 0;

	// Parse out the max values for normalization
	for(trainingFileSuspectMap::iterator it = trainingTable->begin(); it != trainingTable->end(); it++)
	{
		for(int p = it->second->points.size() - 1; p >= 0; p--)
		{
			numTotal++;
			stringstream ss(it->second->points.at(p));
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

	for(trainingFileSuspectMap::iterator it = trainingTable->begin(); it != trainingTable->end(); it++)
	{
		// Can't trim points if there's only 1
		if(it->second->points.size() < 2)
		{
			continue;
		}

		stringstream ss(it->second->points.at(it->second->points.size() - 1));
		for(int d = 0; d < DIM; d++)
		{
			string feature;
			getline(ss, feature, ' ');
			newerPoint[d] = atof(feature.c_str()) / maxValues[d];
		}

		for(int p = it->second->points.size() - 2; p >= 0; p--)
		{
			double distance = 0;

			stringstream ss(it->second->points.at(p));
			for(uint d = 0; d < DIM; d++)
			{
				string feature;
				getline(ss, feature, ' ');
				olderPoint[d] = atof(feature.c_str()) / maxValues[d];
			}

			for(uint d=0; d < DIM; d++)
				distance += annDist(d, olderPoint,newerPoint);

			// Should we throw this point away?
			if(distance < distanceThreshhold)
			{
				it->second->points.erase(it->second->points.begin() + p);
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
