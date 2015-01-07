//============================================================================
// Name        : VendorMacDb.cpp
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
// Description : Class for using the nmap MAC prefix file
//============================================================================

#include "VendorMacDb.h"
#include "../NovaUtil.h"
#include "../Config.h"
#include "../Logger.h"

#include <boost/algorithm/string.hpp>
#include <errno.h>
#include <fstream>
#include <math.h>
#include <sstream>
#include <string.h>

using namespace std;
using namespace Nova;

VendorMacDb::VendorMacDb()
{
	m_macVendorFile =  Config::Inst()->GetPathShared() + "/nmap-mac-prefixes";
}

VendorMacDb::VendorMacDb(string MacVendorFile)
{
	m_macVendorFile = MacVendorFile;
}

void VendorMacDb::LoadPrefixFile()
{
	ifstream MACPrefixes(m_macVendorFile.c_str());
	string line, vendor, prefixStr;
	char *notUsed;
	uint prefix;
	if(MACPrefixes.is_open())
	{
		while(MACPrefixes.good())
		{
			getline(MACPrefixes, line);

			// Skip blank and comment lines
			if (line.length() <= 3 || line.at(0) == '#')
			{
				continue;
			}

			// Skip lines that don't have a space (format is "prefix vendor")
			if (line.find(' ') == string::npos) {
				continue;
			}

			prefixStr = line.substr(0, line.find(' '));
			vendor = line.substr(line.find(' ') + 1, string::npos);

			errno = 0;
			prefix = strtoul(prefixStr.c_str(), &notUsed, 16);
			if(errno)
				continue;

			m_MACVendorTable[prefix] = vendor;
			if(m_vendorMACTable.keyExists(vendor))
			{
				m_vendorMACTable[vendor]->push_back(prefix);
			}
			else
			{
				vector<uint> *vect = new vector<uint>;
				vect->push_back(prefix);
				m_vendorMACTable[vendor] = vect;
			}
		}
	}
	MACPrefixes.close();
}


//Randomly selects one of the ranges associated with vendor and generates the remainder of the MAC address
// *note conflicts are only checked for locally, weird things may happen if the address is already being used.
string VendorMacDb::GenerateRandomMAC(string vendor)
{
	stringstream suffixStr;
	stringstream tempStr;
	char charBuffer[3];
	bzero(charBuffer, 3);
	VendorToMACTable::iterator it = m_vendorMACTable.find(vendor);
	//If we can resolve the vendor to a range
	if(it != m_vendorMACTable.end())
	{
		unsigned char randByte = (unsigned char)(rand() % (int)(pow(2, 8)));
		sprintf(charBuffer, "%02x", randByte);
		tempStr << charBuffer[0] << charBuffer[1];
		suffixStr << tempStr.str();
		suffixStr << ":";

		tempStr.str("");
		bzero(charBuffer, 3);
		randByte = (unsigned char)(rand() % (int)(pow(2, 8)));
		sprintf(charBuffer, "%02x", randByte);
		tempStr << charBuffer[0] << charBuffer[1];
		suffixStr << tempStr.str();
		suffixStr << ":";

		tempStr.str("");
		bzero(charBuffer, 3);
		randByte = (unsigned char)(rand() % (int)(pow(2, 8)));
		sprintf(charBuffer, "%02x", randByte);
		tempStr << charBuffer[0] << charBuffer[1];
		suffixStr << tempStr.str();

		tempStr.str("");
		vector<uint> prefixes = *m_vendorMACTable[vendor];
		uint index = (rand() % prefixes.size());
		uint selectedPrefix = prefixes[index];
		char prefixBuffer[7];
		bzero(prefixBuffer, 7);
		sprintf(prefixBuffer, "%06x", selectedPrefix);
		string prefixString(prefixBuffer);
		tempStr << prefixBuffer[0] << prefixBuffer[1] << ":";
		tempStr << prefixBuffer[2] << prefixBuffer[3] << ":";
		tempStr << prefixBuffer[4] << prefixBuffer[5] << ":";
		tempStr << suffixStr.str();

		uint macPrefix = AtoMACPrefix(tempStr.str());
		if(LookupVendor(macPrefix).compare(vendor))
		{
			LOG(ERROR, "Random MAC Generation failed! " + tempStr.str() , "");
			return "";
		}
		return tempStr.str();
	}
	LOG(DEBUG, "Random MAC Generation failed!", "");
	return "";
}

//Resolve the first 3 bytes of a MAC Address to a MAC vendor that owns the range, returns the vendor string
string VendorMacDb::LookupVendor(uint MACPrefix)
{
	if(m_MACVendorTable.keyExists(MACPrefix))
	{
		return m_MACVendorTable[MACPrefix];
	}
	else
	{
		return "";
	}
}

bool VendorMacDb::IsVendorValid(string vendor)
{
	return m_vendorMACTable.keyExists(vendor);
}

vector<string> VendorMacDb::SearchVendors(string partialVendorName)
{
	vector<string> matches;
	bool matched;

	for(VendorToMACTable::iterator it = m_vendorMACTable.begin(); it != m_vendorMACTable.end(); it++)
	{
		matched = true;
		if(strcasestr(it->first.c_str(), partialVendorName.c_str()) == NULL)
			matched = false;

		if(matched)
			matches.push_back(it->first);
	}

	return matches;
}

vector<string> VendorMacDb::GetVendorNames() {
	vector<string> names;
	for(VendorToMACTable::iterator it = m_vendorMACTable.begin(); it != m_vendorMACTable.end(); it++)
	{
		names.push_back(it->first);
	}
	return names;
}

//Converts the first three bytes of a MAC address from a string to a unsigned int
//	MAC: string of the MAC address you wish to convert must be at least 8 characters
//	return: unsigned integer value of the MAC prefix
//	note: must be valid hex character pairs separated by colons, ex: '09:af:AF'
uint VendorMacDb::AtoMACPrefix(string MAC)
{
	if(MAC.size() < 8)
	{
		return 0;
	}
	u_char tempBuf = 0;
	uint ret = 0;
	for(uint i = 0; (i < MAC.length()) && (i < 8); i++)
	{
		uint val = 0;
		if(isdigit(MAC[i]) && MAC[i] != '0')
		{
			val = MAC[i] - DIGIT_OFFSET;
		}
		else if(islower(MAC[i]) && MAC[i] < 'g')
		{
			val = MAC[i] - LOWER_OFFSET;
		}
		else if(isupper(MAC[i]) && MAC[i] < 'G')
		{
			val = MAC[i] - UPPER_OFFSET;
		}
		else if(MAC[i] != ':' && MAC[i] != '0')
		{
			return 0;
		}
		switch(i % 3)
		{
			case 0:
			{
				tempBuf += val;
				tempBuf = tempBuf << 4;
				break;
			}
			case 1:
			{
				tempBuf += val;
				break;
			}
			case 2:
			{
				ret += tempBuf;
				tempBuf = 0;
				ret = ret << 8;
				break;
			}
			default:
			{
				return 0;
			}
		}
	}
	ret += tempBuf;
	return ret;
}
