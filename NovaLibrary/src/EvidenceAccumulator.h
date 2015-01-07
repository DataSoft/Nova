//============================================================================
// Name        : EvidenceAccumulator.h
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
// Description : Maintains and calculates distinct features for individual Suspects
//					for use in classification of the Suspect.
//============================================================================

#ifndef FEATURESET_H_
#define FEATURESET_H_

#include "Evidence.h"
#include "HashMapStructs.h"

#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

//dimension
#define DIM 14

//Table of IP destinations and a count;
typedef Nova::HashMap<uint32_t, uint64_t, std::hash<time_t>, eqtime > IP_Table;
//Table of destination ports and a count;
typedef Nova::HashMap<in_port_t, uint64_t, std::hash<in_port_t>, eqport > Port_Table;
//Table of packet sizes and a count
typedef Nova::HashMap<uint16_t, uint64_t, std::hash<uint16_t>, eq_uint16_t > Packet_Table;

struct IpPortCombination
{
	uint32_t m_ip;
	uint16_t m_port;
	uint16_t m_internal;

	IpPortCombination()
	{
		m_ip = 0;
		m_port = 0;
		m_internal = 0;
	}

	bool operator ==(const IpPortCombination &rhs) const
	{
		// This is for checking equality of empty/deleted keys
		if (m_internal != 0 || rhs.m_internal != 0)
		{
			return m_internal == rhs.m_internal;
		}
		else
		{
			return (m_ip == rhs.m_ip && m_port == rhs.m_port);
		}
	}

	bool operator != (const IpPortCombination &rhs) const
	{
		return !(this->operator ==(rhs));
	}
};


// Make a IpPortCombination hash and equals function for the Google hash maps
namespace std
{
	template<>
	struct hash< IpPortCombination > {
		std::size_t operator()( const IpPortCombination &c ) const
		{
			uint32_t a = c.m_ip;

			// Thomas Wang's integer hash function
			// http://www.cris.com/~Ttwang/tech/inthash.htm
			a = (a ^ 61) ^ (a >> 16);
			a = a + (a << 3);
			a = a ^ (a >> 4);
			a = a * 0x27d4eb2d;
			a = a ^ (a >> 15);

			const int SECRET_CONSTANT = 104729; // 1,000th prime number

			// Map 16-bit port 1:1 to a random-looking number
			a += ((uint32_t)c.m_port * (SECRET_CONSTANT*4 + 1)) & 0xffff;

			return a;
		}
	};
}

struct IpPortCombinationEquals
{
	bool operator()(IpPortCombination k1, IpPortCombination k2) const
	{
		return k1 == k2;
	}
};


typedef Nova::HashMap<IpPortCombination, uint64_t, std::hash<IpPortCombination>, IpPortCombinationEquals> IpPortTable;

namespace Nova
{


class EvidenceAccumulator
{

public:
	EvidenceAccumulator();

	// Adds evidence to the accumulated data we've gathered for a suspect
	void Add(const Evidence &evidence);


	/// The computed feature values used for KNN
	double m_features[DIM];

	// Names of the KNN feature values we compute
	static std::string m_featureNames[];

	//Number of packets total
	uint64_t m_packetCount;

	uint64_t m_tcpPacketCount;
	uint64_t m_udpPacketCount;
	uint64_t m_icmpPacketCount;
	uint64_t m_otherPacketCount;


	// For some TCP flag ratios and statistics
	uint64_t m_rstCount;
	uint64_t m_ackCount;
	uint64_t m_synCount;
	uint64_t m_finCount;
	uint64_t m_synAckCount;

	time_t m_startTime;
	time_t m_endTime;
	time_t m_lastTime;

	//Table of Packet sizes and counts for variance calc
	Packet_Table m_packTable;

	time_t m_totalInterval;

	//Total number of bytes in all packets
	uint64_t m_bytesTotal;


	IP_Table m_IPTable;

	// Maps IP/port to a bool, used for checking if m_portContactedPerIP needs incrementing for this IP
	IpPortTable m_hasTcpPortIpBeenContacted;
	IpPortTable m_hasUdpPortIpBeenContacted;
	IpPortTable m_icmpCodeTypes;
};
}

#endif /* FEATURESET_H_ */
