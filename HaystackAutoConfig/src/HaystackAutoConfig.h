//============================================================================
// Name        : HoneydHostConfig.h
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
// Description : Header for the HoneydHostConfig.cpp file, defines some variables
//               and provides function declarations
//============================================================================
#ifndef HONEYDHOSTCONFIG_
#define HONEYDHOSTCONFIG_

#include "HashMapStructs.h"
#include "HashMap.h"
#include <boost/property_tree/ptree.hpp>

namespace Nova
{
enum HHC_ERR_CODE : int
{
	HHC_CODE_OKAY = 0,
	HHC_CODE_AUTODETECT_FAIL,
	HHC_CODE_GET_NAMEINFO_FAIL,
	HHC_CODE_GET_BITMASK_FAIL,
	HHC_CODE_NO_MATCHED_PERSONALITY,
	HHC_CODE_PARSING_ERROR,
	HHC_CODE_INCORRECT_NUMBER_ARGS,
	HHC_CODE_NON_INTEGER_ARG,
	HHC_CODE_BAD_ARG_VALUE,
	HHC_CODE_REQUIRED_FLAGS_MISSING,
	HHC_CODE_BAD_FUNCTION_PARAM,
	HHC_CODE_NO_NMAP,
	HHC_CODE_NO_MULTI_RUN,
	HHC_CODE_RECV_SIG,
	HHC_CODE_GENERIC_ERROR
};

// Loads the nmap xml output into a ptree and passes <host> child nodes to ParseHost
//  const std::string &filename - string of the xml filename to read
// Returns nothing
bool LoadNmapXML(const std::string &filename);

// Takes a <host> sub-ptree and parses it for the requisite information, placing said information
// into a Personality object which then gets passed into the PersonalityTable object
//  ptree pt2 - <host> subtree of the highest level node in the nmap xml files
void ParseHost(boost::property_tree::ptree pt2);

// Determines what interfaces are present, and the subnets that they're connected to
//  ErrCode errVar - ptr to an error code variable so that we can inspect it's value afterward
//   after if the vector is empty
// Returns a vector containings strings of the subnet addresses
std::vector<std::string> GetSubnetsToScan(HHC_ERR_CODE *errVar, std::vector<std::string> interfacesToMatch);

// Prints out the subnets that're found during GetSubnetsToScan(errVar)
//  vector<string> recv - vector of subnets found
// Only prints, no return value
void PrintStringVector(std::vector<std::string> recv);

// After the subnets are found, pass recv into this function to do the actual scanning
// and generation of the XML files for parsing
//  vector<string> recv - vector of subnets
// Returns an ErrCode signifying success or failure
HHC_ERR_CODE LoadPersonalityTable(std::vector<std::string> recv);

void GenerateConfiguration();

bool CheckSubnet(std::vector<std::string> &hostAddrStrings, std::string matchStr);

int GetNumberOfIPsInRange(std::string ipRange);

std::string GetReverseIp(std::string ip);

}
#endif
