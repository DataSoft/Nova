//============================================================================
// Name        : Suspect.h
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
// Description : A Suspect object with identifying information and traffic
//					features so that each entity can be monitored and classified.
//============================================================================

#ifndef SUSPECT_H_
#define SUSPECT_H_

#include "Point.h"
#include "Evidence.h"
#include "EvidenceAccumulator.h"
#include "protobuf/marshalled_classes.pb.h"

enum FeatureMode: bool
{
	MAIN_FEATURES = false,
};

namespace Nova
{

enum FeatureIndex: uint8_t
{
	IP_TRAFFIC_DISTRIBUTION = 0,
	PORT_TRAFFIC_DISTRIBUTION,
	PACKET_SIZE_MEAN,
	PACKET_SIZE_DEVIATION,
	DISTINCT_IPS,
	DISTINCT_TCP_PORTS,
	DISTINCT_UDP_PORTS,
	AVG_TCP_PORTS_PER_HOST,
	AVG_UDP_PORTS_PER_HOST,
	TCP_PERCENT_SYN,
	TCP_PERCENT_FIN,
	TCP_PERCENT_RST,
	TCP_PERCENT_SYNACK,
	HAYSTACK_PERCENT_CONTACTED
};

// A Suspect represents a single actor on the network, whether good or bad.
// Suspects are the target of classification and a major part of Nova.
class Suspect
{

public:
	Suspect();
	~Suspect();

	SuspectID_pb GetIdentifier();
	void SetIdentifier(SuspectID_pb id);

	static std::string GetIpString(const SuspectID_pb &id);
	static std::string GetIpString(uint32_t ip);

	std::string GetMACString();
	std::string ToString();
	std::string GetIdString();
	std::string GetIpString();
	std::string GetInterface();

	// Proccesses a packet in m_evidence and puts them into the suspects unsent FeatureSet data
	void ReadEvidence(Evidence *evidence, bool deleteEvidence);


	//Returns a copy of the suspects in_addr
	//Returns: Suspect's in_addr_t or NULL on failure
	in_addr_t GetIpAddress();
	//Sets the suspects in_addr_t
	void SetIpAddress(in_addr_t ip);

	//Returns a copy of the Suspects classification double
	// Returns -1 on failure
	double GetClassification();
	//Sets the suspect's classification
	void SetClassification(double n);

	//Returns the number of hostile neighbors
	int GetHostileNeighbors();
	//Sets the number of hostile neighbors
	void SetHostileNeighbors(int i);

	//Returns the hostility bool of the suspect
	bool GetIsHostile();
	//Sets the hostility bool of the suspect
	void SetIsHostile(bool b);

	//Returns a copy of the suspects FeatureSet
	EvidenceAccumulator GetFeatureSet(FeatureMode whichFeatures = MAIN_FEATURES);

	//Returns the accuracy double of the feature using featureIndex 'fi'
	double GetFeatureAccuracy(FeatureIndex fi);

	//Sets the accuracy double of the feature using featureIndex 'fi'
	void SetFeatureAccuracy(FeatureIndex fi, double d);

	// Get the last time we saw this suspect
	long int GetLastPacketTime();


	bool m_needsClassificationUpdate;

	EvidenceAccumulator m_features;

	std::string m_classificationNotes;

	// Last MAC address we saw for the suspect
	uint64_t m_lastMac;

private:
	SuspectID_pb m_id;

	// Array of values that represent the quality of suspect classification on each feature
	double m_featureAccuracy[DIM];

	// The current classification assigned to this suspect.
	//	0-1, where 0 is almost surely benign, and 1 is almost surely hostile.
	//	-1 indicates no classification or error.
	double m_classification;

	//The number of datapoints flagged as hostile that were matched to the suspect (max val == k in the config)
	int32_t m_hostileNeighbors;

	// Is the classification above the current threshold? IE: What conclusion has the CE come to?
	bool m_isHostile;
};

}

#endif /* SUSPECT_H_ */
