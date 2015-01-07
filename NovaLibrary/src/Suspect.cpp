
//============================================================================
// Name        : Suspect.cpp
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

#include "Suspect.h"
#include "Logger.h"
#include "Config.h"
#include "Database.h"

#include <iomanip>
#include <errno.h>
#include <sstream>

#include <sys/time.h>
#include <unistd.h>

using namespace std;

namespace Nova
{

Suspect::Suspect()
{
	m_hostileNeighbors = 0;
	m_classification = -2;
	m_needsClassificationUpdate = false;
	m_isHostile = false;
	m_classificationNotes = "";

	for(int i = 0; i < DIM; i++)
	{
		m_featureAccuracy[i] = 0;
	}
}

Suspect::~Suspect()
{
}

string Suspect::GetMACString()
{
	stringstream ss;

	ss << std::setw(2) << std::hex << std::setfill('0') << std::uppercase;
	ss << (m_lastMac & 0xFF) << ":";
	ss << ((m_lastMac & 0xFF00) >> 8) << ":";
	ss << ((m_lastMac & 0xFF0000) >> 16) << ":";
	ss << ((m_lastMac & 0xFF000000) >> 24) << ":";
	ss << ((m_lastMac & 0xFF00000000) >> 32) << ":";
	ss << ((m_lastMac & 0xFF0000000000) >> 40);

	return ss.str();
}

string Suspect::GetIpString()
{
	return Suspect::GetIpString(this->m_id);
}

string Suspect::GetIpString(const SuspectID_pb &id)
{
	return Suspect::GetIpString(id.m_ip());
}

string Suspect::GetIpString(uint32_t ip)
{
	string ipString = to_string((ip & 0xFF000000) >> 24) + "." +
			to_string((ip & 0x00FF0000) >> 16) + "." +
			to_string((ip & 0x0000FF00) >> 8) + "." +
			to_string((ip & 0x000000FF) >> 0);
	return ipString;
}

string Suspect::GetIdString()
{
	stringstream ss;
	ss << m_id.m_ifname() << " ";
	ss << GetIpString(m_id);
	return ss.str();
}

string Suspect::GetInterface()
{
	return m_id.m_ifname();
}

string Suspect::ToString()
{
	stringstream ss;
	ss << "Suspect: "<< GetIpString() << "\n";
	ss << " Suspect is ";
	if(!m_isHostile)
		ss << "not ";
	ss << "hostile\n";
	ss <<  " Classification: ";

	if(m_classification == -2)
	{
		ss << " Not enough data\n";
	}
	else
	{
		ss << m_classification <<  "\n";
	}

	ss <<  " Hostile neighbors: " << m_hostileNeighbors << "\n";

	ss << "Classification notes: " << endl << m_classificationNotes << endl;


	for(int i = 0; i < DIM; i++)
	{
		ss << EvidenceAccumulator::m_featureNames[i] << ": " << m_features.m_features[i] << "\n";
	}


	return ss.str();
}

//Just like Consume but doesn't deallocate
void Suspect::ReadEvidence(Evidence *evidence, bool deleteEvidence)
{

	m_lastMac = evidence->m_evidencePacket.srcmac;

	if(m_id.m_ip() == 0)
	{
		m_id.set_m_ip(evidence->m_evidencePacket.ip_src);
		m_id.set_m_ifname(evidence->m_evidencePacket.interface);
	}

	Evidence *curEvidence = evidence, *tempEv = NULL;
	while(curEvidence != NULL)
	{
		m_features.Add(*curEvidence);

		m_id.set_m_ifname(curEvidence->m_evidencePacket.interface);


		tempEv = curEvidence;
		curEvidence = tempEv->m_next;

		if (deleteEvidence)
		{
			delete tempEv;
		}
	}

}

//Returns a copy of the suspects in_addr.s_addr
//Returns: Suspect's in_addr.s_addr
in_addr_t Suspect::GetIpAddress()
{
	return m_id.m_ip();
}

SuspectID_pb Suspect::GetIdentifier()
{
	return m_id;
}

void Suspect::SetIdentifier(SuspectID_pb id)
{
	m_id = id;
}

//Sets the suspects in_addr
void Suspect::SetIpAddress(in_addr_t ip)
{
	m_id.set_m_ip(ip);
}

//Returns a copy of the Suspects classification double
double Suspect::GetClassification()
{
	return m_classification;
}

//Sets the suspect's classification
void Suspect::SetClassification(double n)
{
	m_classification = n;
}


//Returns the number of hostile neighbors
int Suspect::GetHostileNeighbors()
{
	return m_hostileNeighbors;
}
//Sets the number of hostile neighbors
void Suspect::SetHostileNeighbors(int i)
{
	m_hostileNeighbors = i;
}

//Returns the hostility bool of the suspect
bool Suspect::GetIsHostile()
{
	return m_isHostile;
}
//Sets the hostility bool of the suspect
void Suspect::SetIsHostile(bool b)
{
	m_isHostile = b;
}


//Returns a copy of the suspects FeatureSet
EvidenceAccumulator Suspect::GetFeatureSet(FeatureMode whichFeatures)
{
	switch(whichFeatures)
	{
		default:
		case MAIN_FEATURES:
		{
			return m_features;
			break;
		}
	}
}

//Returns the accuracy double of the feature using featureIndex 'fi'
double Suspect::GetFeatureAccuracy(FeatureIndex fi)
{
	return m_featureAccuracy[fi];
}
//Sets the accuracy double of the feature using featureIndex 'fi'
void Suspect::SetFeatureAccuracy(FeatureIndex fi, double d)
{
	m_featureAccuracy[fi] = d;
}

}
