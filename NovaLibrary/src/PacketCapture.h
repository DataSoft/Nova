//============================================================================
// Name        : PacketCapture.h
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

#ifndef PACKETCAPTURE_H_
#define PACKETCAPTURE_H_

#include <string>
#include <pcap.h>

namespace Nova
{

class PacketCapture
{
public:
	PacketCapture();
	virtual ~PacketCapture();

	void SetPacketCb(void (*cb)(unsigned char *index, const struct pcap_pkthdr *pkthdr, const unsigned char *packet));
	void SetFilter(std::string filter);

	pcap_t* GetPcapHandle();

	bool StartCapture();
	bool StartCaptureBlocking();

	void StopCapture();

	int GetDroppedPackets();

	// This is the pcap API id that's passed to the packet capture callback
	u_char GetIdIndex() {return m_index;}
	void SetIdIndex(u_char index) {m_index = index;}

	std::string GetIdentifier() {return m_identifier;}
	void SetIdentifier(std::string identifier) {m_identifier = identifier;}

	pcap_t *m_handle;
protected:
	std::string m_identifier;
	std::string m_filter;
	u_char m_index;

	volatile bool isCapturing;
	volatile bool stoppingCapture;
	pthread_mutex_t stoppingMutex;

	// This is so we can run blocking pcap_loop in it's own thread
	pthread_t m_thread;
	char m_errorbuf[PCAP_ERRBUF_SIZE];

	void InternalThreadEntry();

	// Work around for conversion of class method to C style function pointer for pcap
	void (*m_packetCb)(unsigned char *index, const struct pcap_pkthdr *pkthdr, const unsigned char *packet);
	static void * InternalThreadEntryFunc(void * This)
	{
		((PacketCapture*)This)->InternalThreadEntry();
		return NULL;
	}

	// This is to signal the internal thread to stop sleeping
	static void SleepStopper(int signum)
	{
		return;
	}

};

class PacketCaptureException : public std::exception
{
public:
	std::string s;
	PacketCaptureException(std::string ss) : s(ss) {}
	~PacketCaptureException() throw () {}

	const char* what() const throw()
	{
		return s.c_str();
	}
};

} /* namespace Nova */
#endif /* PACKETCAPTURE_H_ */
