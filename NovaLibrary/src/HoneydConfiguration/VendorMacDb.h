//============================================================================
// Name        : MacAddressDb.h
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
// Description : Object for using the nmap MAC prefix file
//============================================================================


#ifndef MACADDRESSDB_H_
#define MACADDRESSDB_H_

#include "../HashMapStructs.h"

#include <string>
#include <vector>

#define DIGIT_OFFSET 48
#define LOWER_OFFSET 87
#define UPPER_OFFSET 65

typedef Nova::HashMap<uint, std::string, std::hash<uint>, eqint> MACToVendorTable;
typedef Nova::HashMap<std::string, std::vector<uint> *,  std::hash<std::string>, eqstr > VendorToMACTable;

class VendorMacDb
{
public:
	VendorMacDb();

	// Uses a user defined vendor -> mac prefix file
	//		macVendorFile: Prefix file path. Contains MAC prefix followed by the name of the vendor
	VendorMacDb(std::string macVendorFile);

	// Loads the prefix file and populates it's internal state
	void LoadPrefixFile();

	// Generates a random MAC address for a vendor
	//		vendor: Name of vendor to use for the prefix
	std::string GenerateRandomMAC(std::string vendor);

	// Finds a vendor based on a MAC prefix
	std::string LookupVendor(uint MACPrefix);

	// Searches for a vendor that matches a std::string
	//		partialVendorName: Any part of the vendor name
	std::vector <std::string> SearchVendors(std::string partialVendorName);

	// Gets list of all the vendor names
	std::vector <std::string> GetVendorNames();

	// Checks if a vendor name is known
	bool IsVendorValid(std::string vendor);

	// Puts the integer val into m, and shifts it
	// depending on the value of cond. Used for converting
	// a MAC address string into a uint for use with the MAC Vendor Db object.
	uint AtoMACPrefix(std::string MAC);


private:
	MACToVendorTable m_MACVendorTable;
	VendorToMACTable m_vendorMACTable;
	std::string m_macVendorFile;
};


#endif /* MACADDRESSDB_H_ */
