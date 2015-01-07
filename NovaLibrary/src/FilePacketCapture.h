//============================================================================
// Name        : FilePacketCapture.h
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

#ifndef FILEPACKETCAPTURE_H_
#define FILEPACKETCAPTURE_H_

#include "PacketCapture.h"

namespace Nova
{

class FilePacketCapture: public Nova::PacketCapture
{
public:
	FilePacketCapture(std::string pcapFilePath);
	void Init();

private:
	std::string m_pcapFilePath;

};

} /* namespace Nova */
#endif /* FILEPACKETCAPTURE_H_ */
