//============================================================================
// Name        : ThresholdTriggerClassification.cpp
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
// Description : Child class of ClassificationEngine, specifies a threshold for
// classifying a device as an intruder
//============================================================================

#include <boost/algorithm/string.hpp>
#include <fstream>
#include <sstream>
#include <string>

#include "Logger.h"
#include "Config.h"
#include "ThresholdTriggerClassification.h"

using namespace std;

namespace Nova
{

ThresholdTriggerClassification::ThresholdTriggerClassification()
{

}

void ThresholdTriggerClassification::LoadConfiguration(string filePath)
{
	ifstream settings(filePath);
	string prefix, line;

	if(settings.is_open())
	{
		while(settings.good())
		{
			getline(settings,line);

			prefix = "THRESHOLD_HOSTILE_TRIGGERS";
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_hostileThresholds.clear();

					boost::algorithm::trim(line);
					vector<string> thresholds;
					boost::split(thresholds, line, boost::is_any_of("\t "));

					if (thresholds.size() != DIM)
					{
						stringstream ss;
						ss << "THRESHOLD_HOSTILE_TRIGGERS does not contain the correct number of entries. Should have " << DIM << " but has " << thresholds.size();
						LOG(CRITICAL, ss.str(), "");
						exit(EXIT_FAILURE);
						continue;
					}

					for (uint i = 0; i < thresholds.size(); i++)
					{
						HostileThreshold setting;
						setting.m_hasMaxValueTrigger = false;
						setting.m_hasMinValueTrigger = false;

						if (thresholds.at(i).at(0) == '-')
						{

						}
						else if (thresholds.at(i).at(0) == '>')
						{
							// Check if this has both a > and a < symbol
							vector<string> parts;
							boost::split(parts, thresholds.at(i), boost::is_any_of("<"));
							if (parts.size() == 2)
							{
								string maxValueString = parts.at(0).substr(1, string::npos);
								istringstream s1(maxValueString);
								if (!(s1 >> setting.m_maxValueTrigger))
								{
									LOG(ERROR, "Unable to parse max value for THRESHOLD_HOSTILE_TRIGGERS", "");
								}
								else
								{
									setting.m_hasMaxValueTrigger = true;
								}


								string minValueString = parts.at(1);
								istringstream s2(minValueString);
								if (!(s2 >> setting.m_minValueTrigger))
								{
									LOG(ERROR, "Unable to parse min value for THRESHOLD_HOSTILE_TRIGGERS", "");
								}
								else
								{
									setting.m_hasMinValueTrigger = true;
								}
							}
							else
							{
								istringstream s(thresholds.at(i).substr(1, string::npos));
								if (!(s >> setting.m_maxValueTrigger))
								{
									LOG(ERROR, "Unable to parse max value for THRESHOLD_HOSTILE_TRIGGERS", "");
								}
								else
								{
									setting.m_hasMaxValueTrigger = true;
								}
							}

						}
						else if (thresholds.at(i).at(0) == '<')
						{
							istringstream s(thresholds.at(i).substr(1, thresholds.at(i).npos));
							if (!(s >> setting.m_minValueTrigger))
							{
								LOG(ERROR, "Unable to parse min value for THRESHOLD_HOSTILE_TRIGGERS", "");
							}
							else
							{
								setting.m_hasMinValueTrigger = true;
							}
						}

						m_hostileThresholds.push_back(setting);
					}
				}
				continue;
			}

		}
	} else {
		LOG(CRITICAL, "Unable to load configuration file for classification engine at " + filePath, "");
		exit(EXIT_FAILURE);
	}

	settings.close();
}

double ThresholdTriggerClassification::Classify(Suspect *suspect)
{
	double classification = 0;

	suspect->m_classificationNotes += "=== Notes from threshold based classification engine ===\n";

	//vector<HostileThreshold> thresholds = Config::Inst()->GetHostileThresholds();
	for(uint i = 0; i < DIM; i++)
	{
		HostileThreshold threshold = m_hostileThresholds.at(i);
		if (threshold.m_hasMaxValueTrigger)
		{
			if (suspect->m_features.m_features[i] >= threshold.m_maxValueTrigger)
			{
				suspect->m_classificationNotes += "Feature '" + EvidenceAccumulator::m_featureNames[i] + "' has surpassed threshold of " + to_string(threshold.m_maxValueTrigger) + "\n";
				classification = 1;
			}
		}

		if (threshold.m_hasMinValueTrigger)
		{
			if (suspect->m_features.m_features[i] <= threshold.m_minValueTrigger)
			{
				suspect->m_classificationNotes += "Feature '" + EvidenceAccumulator::m_featureNames[i] + "' is below threshold of " + to_string(threshold.m_minValueTrigger) + "\n";
				classification = 1;
			}
		}
	}

	if (!classification)
	{
		suspect->m_classificationNotes += "No threshold alerts triggered\n";
	}

	if (classification > Config::Inst()->GetClassificationThreshold())
	{
		suspect->SetIsHostile(true);
	}
	else
	{
		suspect->SetIsHostile(false);
	}
	suspect->SetClassification(classification);

	return classification;
}

} /* namespace Nova */
