//============================================================================
// Name        : Evidence.cpp
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

#include <dumbnet.h>
#include <string.h>
#include <iostream>
#include "Evidence.h"
#include "netinet/tcp.h"
#include <netinet/if_ether.h>

using namespace std;

namespace Nova
{

//Default Constructor
Evidence::Evidence()
{
	m_next = NULL;
	m_evidencePacket.srcmac = 0;
	m_evidencePacket.dst_port = 0;
	m_evidencePacket.ip_dst = 0;
	m_evidencePacket.ip_len = 0;
	m_evidencePacket.ip_p = 0;
	m_evidencePacket.ip_src = ~0;
	m_evidencePacket.ts = 0;
}

Evidence::Evidence(const u_char *packet, const pcap_pkthdr *pkthdr)
{
	struct eth_hdr *eth = (eth_hdr *)packet;
	memcpy(((char*)&m_evidencePacket.srcmac), eth->eth_src.data, 6);

	const u_char *packet_at_ip_header = packet + ETH_HDR_LEN;
	struct ip_hdr *ip;
	ip = (ip_hdr *) packet_at_ip_header;

	m_evidencePacket.ts = pkthdr->ts.tv_sec;

	uint8_t ip_hl = ip->ip_hl;
	m_next = NULL;

	m_evidencePacket.ip_len = ntohs(ip->ip_len);
	m_evidencePacket.ip_p = ip->ip_p;
	m_evidencePacket.ip_src = ntohl(ip->ip_src);
	m_evidencePacket.ip_dst = ntohl(ip->ip_dst);

	m_evidencePacket.dst_port = -1;

	if((m_evidencePacket.ip_p == IPPROTO_TCP))
	{
		struct tcp_hdr *tcp = (tcp_hdr*) (packet_at_ip_header + ip_hl*4);

		m_evidencePacket.dst_port = ntohs(tcp->th_dport);

		// Read in the tcp flags
		m_evidencePacket.tcp_hdr.ack = tcp->th_flags & TH_ACK;
		m_evidencePacket.tcp_hdr.rst = tcp->th_flags & TH_RST;
		m_evidencePacket.tcp_hdr.syn = tcp->th_flags & TH_SYN;
		m_evidencePacket.tcp_hdr.fin = tcp->th_flags & TH_FIN;
	}
	else if(m_evidencePacket.ip_p == IPPROTO_UDP)
	{
		struct udp_hdr *udp = (udp_hdr*) (packet_at_ip_header + ip_hl*4);
		m_evidencePacket.dst_port = ntohs(udp->uh_dport);
	}
	else if (m_evidencePacket.ip_p == IPPROTO_ICMP)
	{
		struct icmp_hdr *hdr = (icmp_hdr*) (packet_at_ip_header + ip_hl*4);
		m_evidencePacket.dst_port = (uint16_t)0 | (uint16_t)hdr->icmp_type | ((uint16_t)hdr->icmp_code << 8);
	}



}

Evidence::Evidence(Evidence *evidence)
{
	m_evidencePacket = evidence->m_evidencePacket;
	m_next = NULL;
}

}
