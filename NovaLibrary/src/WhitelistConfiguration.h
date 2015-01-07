//============================================================================
// Name        : WhitelistConfiguration.h
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
// Description : Configuration for the whitelist
//============================================================================

#ifndef WHITELISTCONFIGURATION_H_
#define WHITELISTCONFIGURATION_H_

#include <string>
#include <vector>
#include <algorithm>

namespace Nova
{

class WhitelistConfiguration
{
public:
	static bool AddEntry(std::string entry);
	static bool AddIp(std::string interface, std::string ip);
	static bool AddIpRange(std::string interface, std::string ip, std::string netmask);

	static bool DeleteEntry(std::string entry);

	static std::vector<std::string> GetIps();
	static std::vector<std::string> GetIpRanges();

	// Some simple abstraction functions to split a IP/subnet line
	static std::string GetInterface(std::string whitelistEntry);
	static std::string GetSubnet(std::string whitelistEntry);
	static std::string GetIp(std::string whitelistEntry);
	static inline std::string &rtrim(std::string &s) {
	        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	        return s;
	}

private:
	// Gets either IP addresses or IP + netmasks
	static std::vector<std::string> GetWhitelistedIps(bool getRanges);

};

}
#endif /* WHITELISTCONFIGURATION_H_ */
