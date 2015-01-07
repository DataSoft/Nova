//============================================================================
// Name        : ClassificationEngine.h
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
// Description : Suspect classification engine
//============================================================================

#ifndef KNNCLASSIFICATIONENGINE_H_
#define KNNCLASSIFICATIONENGINE_H_

#include <string>
#include <fstream>

#include "ANN/ANN.h"

#include "Logger.h"
#include "Suspect.h"
#include "Doppelganger.h"
#include "ClassificationEngine.h"

namespace Nova
{
class KnnClassification : public Nova::ClassificationEngine
{
public:
	KnnClassification();

	~KnnClassification();

	// Performs classification on given suspect
	//		suspect - suspect to classify based on current evidence
	// 		returns: suspect classification
	// Note: this updates the classification of the suspect in dataPtsWithClass as well as it's isHostile variable
	double Classify(Suspect *suspect);

	// Reads into the list of suspects from a file specified by inFilePath
	//		inFilePath - path to input file, should contain Feature dimensions
	//					 followed by hostile classification (0 or 1), all space separated
	void LoadDataPointsFromFile(std::string inFilePath);
	void LoadDataPointsFromVector(std::vector<double*> points);

	// Normalized a single value
	static double Normalize(NormalizationType type, double value, double min, double max, double weight);

	void LoadConfiguration(std::string filePath);

private:
	// Types of normalization to apply to our features
	std::vector<NormalizationType> m_normalization;

	std::vector <Point*> m_dataPtsWithClass;

	// kdtree stuff
	int m_nPts;						//actual number of data points
	ANNpointArray m_dataPts;				//data points
	ANNpointArray m_normalizedDataPts;	//normalized data points
	ANNkd_tree*	m_kdTree;					// search structure

	pthread_rwlock_t m_lock;

	// Used for data normalization
	double m_maxFeatureValues[DIM];
	double m_minFeatureValues[DIM];
	double m_meanFeatureValues[DIM];

	bool m_isFeatureEnabled[DIM];
	uint m_enabledFeatureCount;
	double m_squrtEnabledFeatures;
	std::string m_pathTrainingFile;

	std::vector<double> m_featureWeights;
};

} // End namespace

#endif /* CLASSIFICATIONENGINE_H_ */
