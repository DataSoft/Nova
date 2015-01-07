//============================================================================
// Name        : PacketCapture.cpp
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
// Description : A simple C++ wrapper class for libpcap packet captures
//============================================================================

#include "PacketCapture.h"
#include "Logger.h"
#include "Lock.h"

#include <pthread.h>
#include <signal.h>
#include <unistd.h>

using namespace Nova;
using namespace std;

PacketCapture::PacketCapture()
{
	m_handle = NULL;
	m_packetCb = NULL;
	isCapturing = false;
	stoppingCapture = false;
	pthread_mutex_init(&this->stoppingMutex, NULL);
}

PacketCapture::~PacketCapture()
{

}


void PacketCapture::SetPacketCb(void (*cb)(unsigned char *index, const struct pcap_pkthdr *pkthdr, const unsigned char *packet))
{
	m_packetCb = cb;
}

void PacketCapture::SetFilter(string filter)
{
	m_filter = filter;

	struct bpf_program fp;

	if(pcap_compile(m_handle, &fp, m_filter.c_str(), 0, PCAP_NETMASK_UNKNOWN) == -1)
	{
		throw PacketCaptureException("Couldn't parse filter: "+m_filter+ " " + pcap_geterr(m_handle) +".");
	}

	if(pcap_setfilter(m_handle, &fp) == -1)
	{
		throw PacketCaptureException("Couldn't install filter: "+m_filter+ " " + pcap_geterr(m_handle) +".");
	}
}

pcap_t* PacketCapture::GetPcapHandle()
{
	return m_handle;
}

int PacketCapture::GetDroppedPackets()
{
	if (m_handle == NULL)
	{
		return 0;
	}

	if (!isCapturing)
	{
		return 0;
	}

	pcap_stat captureStats;
	int result = pcap_stats(m_handle, &captureStats);

	if (result != 0)
	{
		return -1;
	}
	else
	{
		return captureStats.ps_drop;
	}
}

bool PacketCapture::StartCapture()
{
	LOG(DEBUG, "Starting packet capture on: " + m_identifier, "");
	return (pthread_create(&m_thread, NULL, InternalThreadEntryFunc, this) == 0);
}

bool PacketCapture::StartCaptureBlocking()
{
	LOG(DEBUG, "Starting packet capture on: " + m_identifier, "");
	return (pcap_loop(m_handle, -1, m_packetCb, reinterpret_cast<u_char*>(this)) == 0);
}

void PacketCapture::StopCapture()
{
	// Kill and wait for the child thread to exit
	{
		Lock(&this->stoppingMutex);
		stoppingCapture = true;
	}
	pcap_breakloop(m_handle);
	pthread_kill(m_thread, SIGUSR2);
	pthread_join(m_thread, NULL);

	pcap_close(m_handle);
	m_handle = NULL;

	{
		Lock(&this->stoppingMutex);
		stoppingCapture = false;
	}

	pthread_mutex_destroy(&this->stoppingMutex);
}

void PacketCapture::InternalThreadEntry()
{
	signal(SIGUSR2, SleepStopper);
	while (true)
	{
		Lock(&this->stoppingMutex);
		if (stoppingCapture)
		{
			break;
		}

		int activationReturnValue = pcap_activate(m_handle);
		SetFilter(m_filter);

		if (activationReturnValue == 0 || activationReturnValue == PCAP_ERROR_ACTIVATED)
		{
			isCapturing = true;
			int loopReturn = pcap_loop(m_handle, -1, m_packetCb, reinterpret_cast<u_char*>(this));
			isCapturing = false;

			if (loopReturn == -1)
			{
				LOG(ERROR, "Dropped out of pcap loop because of error for packet capture: " + m_identifier + ". Error was: " + string(pcap_geterr(m_handle)), "");
				sleep(10);

				// Try to reactivate the interface on the next loop around
			}
			else if (loopReturn >= 0 || loopReturn == -2)
			{
				// Normal exit case. If a pcap file, it reached the end. If an interface, someone called pcap_breakloop
				LOG(DEBUG, "Dropped out of pcap loop normally for packet capture: " + m_identifier, "");
				break;
			}
			else
			{
				// I've seen pcap_loop return -3... this isn't documented in the manual. Just assume we can't recover from this and break out of the loop
				LOG(ERROR, "Dropped out of pcap loop because of unexpected error for packet capture: " + m_identifier + ". Error was: " + string(pcap_geterr(m_handle)), "");
				break;
			}
		}
		else if (activationReturnValue == PCAP_ERROR_IFACE_NOT_UP)
		{
			LOG(ERROR, "Can't activate packet capture on: " + m_identifier + ". Error was: " + string(pcap_geterr(m_handle)), "");
			sleep(10);
		}
	}
}
