//============================================================================
// Name        : Broadcast.h
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

#ifndef BROADCAST_H
#define BROADCAST_H

#include <string>

namespace Nova
{

class Broadcast {
public:
	Broadcast();

	int m_srcPort;
	int m_dstPort;
	int m_time;
	std::string m_script;

	int GetSrcPort() { return m_srcPort;}
	int GetDstPort() { return m_dstPort;}
	int GetTime() { return m_time;}
	std::string GetScript() { return m_script; }
};


}

#endif
