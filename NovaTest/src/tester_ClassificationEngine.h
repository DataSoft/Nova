//============================================================================
// Name        : tester_KnnClassification.h
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
// Description : This file contains unit tests for the class KnnClassification
//============================================================================/*

#include "gtest/gtest.h"
#include <iostream>
#include <fstream>

#include "KnnClassification.h"

using namespace Nova;
using namespace std;

// The test fixture for testing class KnnClassification.
class KnnClassificationTest : public ::testing::Test
{

protected:

	KnnClassification *testObject;

	// Unused methods here may be deleted
	KnnClassificationTest()
	{
		testObject = new KnnClassification();
	}
};

// Check that someMethod functions
TEST_F(KnnClassificationTest, test_someMethod)
{
	bool isDmEn = Config::Inst()->GetIsDmEnabled();
	Config::Inst()->SetIsDmEnabled(false);
	EXPECT_EQ(0.42, KnnClassification::Normalize(LINEAR, 42, 0, 100, 1));
	Config::Inst()->SetIsDmEnabled(isDmEn);
}

/*
TEST_F(KnnClassificationTest, DISABLED_test_kFoldCrossValidation)
{
	chdir(Config::Inst()->GetPathHome().c_str());

	vector<double*> trainingData;

	ifstream trainingFile;
	trainingFile.open(Config::Inst()->GetPathTrainingFile());

	EXPECT_TRUE(trainingFile.is_open());

	string line;
	char *cLine;
	char *token;
	double *point;


	while (getline(trainingFile, line)) {
		cLine = (char*)line.c_str();
		token = strtok(cLine, " ");

		point = new double[DIM + 1];
		int i = 0;
		while (token != NULL)
		{
			point[i] = atof(token);

			token = strtok (NULL, " ");
			i++;
		}
		trainingData.push_back(point);
	}


	#define FOLDS 10


	// Shuffle our data
	srand ( time(NULL) );
	for (uint i = 0; i < trainingData.size(); i++)
	{
		int r = rand()%trainingData.size();
		double* tmp = trainingData.at(r);
		trainingData.at(r) = trainingData.at(i);
		trainingData.at(i) = tmp;
	}

	Suspect testSuspect;
	FeatureSet fs;

	for (int mask = 1; mask < 1024; mask++)
	{
		char enableMask[DIM];
		int enabledFeatureCount = 0;

		for (int d = 0; d < DIM; d++)
		{
			if (mask & (1 << d))
			{
				enableMask[d] = '1';
				enabledFeatureCount++;
			}
			else
			{
				enableMask[d] = '0';
			}
		}

		if (enabledFeatureCount != 3)
		{
			continue;
		}

		for (int d = 0; d < DIM; d++)
		{
			if (mask & (1 << d))
			{
				cout << FeatureSet::m_featureNames[d] << "+";
			}
		}


		Config::Inst()->SetEnabledFeatures(string(enableMask));

		double falseRatio = 0;
		//double falsePositiveRatio = 0, falseNegativeRatio = 0;
		int falsePositives = 0, falseNegatives = 0;

		for (uint i = 1; i <= FOLDS; i++)
		{
			int foldStart = (i - 1)*((double)trainingData.size()/(double)FOLDS);
			int foldEnd = i*((double)trainingData.size()/(double)FOLDS) - 1;

			vector<double*> subset = trainingData;
			if (foldStart == foldEnd)
			{
				subset.erase(subset.begin() + foldStart);
			}
			else
			{
				subset.erase(subset.begin() + foldStart, subset.begin() + foldEnd);
			}


			testObject->LoadDataPointsFromVector(subset);

			for (int point = foldStart; point <= foldEnd; point++)
			{
				for (int j = 0; j < DIM; j++)
				{
					fs.m_features[j] = trainingData.at(point)[j];
				}

				testSuspect.SetFeatureSet(&fs, MAIN_FEATURES);
				testObject->Classify(&testSuspect);

				if (testSuspect.GetIsHostile() && (trainingData.at(point)[DIM] == 0))
				{
					falsePositives++;
				}

				if (!testSuspect.GetIsHostile() && (trainingData.at(point)[DIM] == 1))
				{
					falseNegatives++;
				}

			}

		}

		//falsePositiveRatio = (double)falsePositives / (double)trainingData.size();
		//falseNegativeRatio = (double)falseNegatives / (double)trainingData.size();
		falseRatio = 1 - (double)(falseNegatives + falsePositives)/ (double)trainingData.size();


			cout << "False positives: " << falsePositives << endl;
			cout << "False positives: " << falsePositiveRatio*100 << "%" << endl;
			cout << "False negatives: " << falseNegatives << endl;
			cout << "False negatives: " << falseNegativeRatio*100 << "%" << endl;
			cout << "Overall classification accuracy: " << falseRatio*100 << endl;
			cout << FeatureSet::m_featureNames[takeAwayDim] << " + " << FeatureSet::m_featureNames[takeAwayDim2] << " " << falseRatio*100  << endl;

		cout << "," << falseRatio*100  << endl;
	}
}
*/
