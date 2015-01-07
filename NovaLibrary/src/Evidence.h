//============================================================================
// Name        : Evidence.h
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
// Description : Evidence object represents a preprocessed ip packet
//					for inclusion in a Suspect's Feature Set
//============================================================================/*

#ifndef EVIDENCE_H_
#define EVIDENCE_H_

#include <string>
#include <pcap.h>
#include <arpa/inet.h>

namespace Nova
{

struct _tcpFlags
{
	bool ack : 1;
	bool rst : 1;
	bool syn : 1;
	bool fin : 1;
};

struct _evidencePacket // Total of 18 bytes
{
	std::string interface;

	uint64_t srcmac;
	uint16_t ip_len; 	//Length in bytes
	uint8_t ip_p;		//Ip protocol (UDP, TCP or ICMP)
	uint32_t ip_src;	//Source IPv4 address
	uint32_t ip_dst;	//Destination IPv4 address
	uint16_t dst_port;	//Destination Port (UDP or TCP Only) or type/code if ICMP
	time_t ts;		//Arrival Timestamp (in seconds)
	_tcpFlags tcp_hdr;
};

class Evidence
{

public:

	_evidencePacket m_evidencePacket;
	Evidence *m_next;

	Evidence(const u_char *packet_at_ip_header, const pcap_pkthdr *pkthdr);

	Evidence(Evidence *evidence);

	Evidence();
};

// The GenericQueue requires an m_next pointer, so this is just so
// we can have a GenericQueue of IP addresses. This ends up having
// a slight performance advantage over std::queue/std::dequeue.
class IpWrapper
{
public:
	IpWrapper *m_next;
	uint32_t ip;

	IpWrapper(uint32_t ip)
	{
		m_next = NULL;
		this->ip = ip;
	}
};

}

#endif /* EVIDENCE_H_ */
