//============================================================================
// Name        : FilePacketCapture.cpp
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
// Description : 
//============================================================================

#include "FilePacketCapture.h"

using namespace std;

namespace Nova
{

FilePacketCapture::FilePacketCapture(string pcapFilePath)
{
	m_pcapFilePath = pcapFilePath;
	m_identifier = pcapFilePath;
}

void FilePacketCapture::Init()
{
	m_handle = pcap_open_offline(m_pcapFilePath.c_str(), m_errorbuf);

	if (m_handle == NULL)
	{
		throw PacketCaptureException(string("pcap_open_offline failed with error: ") + string(m_errorbuf));
	}
}

} /* namespace Nova */
