//============================================================================
// Name        : InterfacePacketCapture.cpp
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
// Description : A simple C++ wrapper class for libpcap interface captures
//============================================================================

#include "InterfacePacketCapture.h"
#include "Config.h"

using namespace std;

namespace Nova
{

InterfacePacketCapture::InterfacePacketCapture(string interface)
{
	m_interface = interface;
	m_identifier = interface;
}

void InterfacePacketCapture::Init()
{
	m_handle = pcap_create(m_interface.c_str(), m_errorbuf);

	if(m_handle == NULL)
	{
		throw PacketCaptureException("Unable to open network interfaces for live capture: "+string(m_errorbuf));
	}

	if(pcap_set_promisc(m_handle, 1) != 0)
	{
		throw PacketCaptureException(string("Unable to set interface mode to promisc due to error: ") + pcap_geterr(m_handle));
	}

	// Set the packet capture buffer size
	if(pcap_set_buffer_size(m_handle, Config::Inst()->GetCaptureBufferSize()) != 0)
	{
		throw PacketCaptureException(string("Unable to set pcap capture buffer size due to error: ") + pcap_geterr(m_handle));
	}

	//Set a capture length of 1Kb. Should be more than enough to get the packet headers
	// 88 == Ethernet header (14 bytes) + max IP header size (60 bytes)  + 4 bytes to extract the destination port for udp and tcp packets
	if(pcap_set_snaplen(m_handle, 88) != 0)
	{
		throw PacketCaptureException(string("Unable to set pcap capture length due to error: ") + pcap_geterr(m_handle));
	}

	if(pcap_set_timeout(m_handle, 1000) != 0)
	{
		throw PacketCaptureException(string("Unable to set pcap timeout value due to error: ") + pcap_geterr(m_handle));
	}

	/*
	if(pcap_activate(m_handle) != 0)
	{
		throw PacketCaptureException(string("Unable to activate packet capture due to error: ") + pcap_geterr(m_handle));
	}*/
}

} /* namespace Nova */
