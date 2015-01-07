//============================================================================
// Name        : NovaUtil.h
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
// Description : Utility functions that are used by multiple components of nova
//					but do not warrant an object
//============================================================================

#ifndef NOVAUTIL_H_
#define NOVAUTIL_H_

#include <string>
#include <vector>
#include <sstream>
#include <netdb.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define BOOST_FILESYSTEM_VERSION 2
#include <boost/filesystem.hpp>

#include "sys/types.h"

namespace Nova
{

// Gets local IP address for interface
//		dev - Device name, e.g. "eth0"
// Returns: IP addresses
std::string GetLocalIP(const char *dev);
std::string GetLocalIP(std::string dev);

std::string GetSubnetFromInterface(std::string interface);

//Removes any instance of the specified character from the front and back of the string
//		str - pointer to the string you want to modify
// 		c - character you wish to remove (Whitespace by default)
// Note: this function will result in an empty string, if every character is == c
void Trim(std::string& str, char c = ' ');


inline std::string ConvertInt(int x)
{
	std::stringstream ss;
	ss << x;
	return ss.str();
}

inline std::string GetEnvVariable( const std::string & var ) {
     const char * val = ::getenv( var.c_str() );
     if ( val == 0 )
     {
         return "";
     }
     else
     {
         return val;
     }
}

//Replaces all instances of the search character in the addressed string with the character specified
//	str: the string to modify
//	searchChar: the character to replace
//	replaceVal: the replacement character
void ReplaceChar(std::string& str, char searchChar, char replaceVal);
void ReplaceString(std::string& str, const std::string& oldStr, const std::string& newStr);

//Takes the input vector of doubles, and changes the double at the index to the target value, then shifts
// all other values in the vector proportionally such that the entire vector sums to 100
//	inputDoubles: The entire list of doubles
//	targetIndex: The index in the vector to change, defaults to 0 if not provided
//	targetValue: The value to shift the double at targetIndex to.
//**Note**: if inputDoulbes does not sum to 100, it will be modified so that it does before calculation
//	ie. ShiftDistribution(vector<4, 4, 2>, 30, 2) will pre-calc the vector<40,40,20> and return <35, 35, 30>
std::vector<double> ShiftDistribution(std::vector<double> inputDoubles, double targetValue, int targetIndex = 0);

//This function takes the vector of doubles and rounds them to integer values preserving the integer sum of the vector
// ie. if this were a vector of percentages <33.333, 33.333, 33.333> it would return <33,34,33>
//	inputDoulbes: The list of doubles to round
std::vector<int> RoundDistributionToIntegers(std::vector<double> inputDoubles);


bool RecursiveDirectoryCopy(boost::filesystem::path const& from, boost::filesystem::path const& to, bool);
}

#endif /* NOVAUTIL_H_ */
