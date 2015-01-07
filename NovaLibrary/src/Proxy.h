//============================================================================
// Name        : Proxy.h
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

#ifndef PROXY_H_
#define PROXY_H_

#include <string>

namespace Nova
{

class Proxy {
public:
	Proxy();

	int m_honeypotPort;
	std::string m_proxyIP;
	int m_proxyPort;
	std::string m_protocol;

	int GetHoneypotPort() {return m_honeypotPort;}
	std::string GetProxyIP() {return m_proxyIP;}
	int GetProxyPort() {return m_proxyPort;}
	std::string GetProtocol() {return m_protocol;}
};

}

#endif /* PROXY_H_ */
