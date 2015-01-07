/*
 * This an example unit test for a fake class VendorMacDb to show the general layout of things.
 * You can copy/paste this and use it as a template for making a new unit test.
 * Much of this is a style thing rather than hard and fast rules for how to set up a test.
 *

//============================================================================
// Name        : tester_VendorMacDb.h
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
// Description : This file contains unit tests for the class VendorMacDb
//============================================================================*/

#include "gtest/gtest.h"
#include "HoneydConfiguration/VendorMacDb.h"

using namespace Nova;

// The test fixture for testing class VendorMacDb.
class VendorMacDbTest : public ::testing::Test
{

protected:

	VendorMacDb m_ethVendors;
	std::vector<std::string> m_vendorNames;
	std::vector<std::string> m_expectedVendors;

	VendorMacDbTest()
	{
		m_ethVendors.LoadPrefixFile();
		m_vendorNames = m_ethVendors.GetVendorNames();
	}
};

using namespace std;

TEST_F(VendorMacDbTest, test_IsVendorValid)
{
	EXPECT_FALSE(m_ethVendors.IsVendorValid("bloopwidgetstuff"));
	EXPECT_TRUE(m_ethVendors.IsVendorValid("Intel"));
}

TEST_F(VendorMacDbTest, test_GetAndSearchVendorNames)
{
	m_expectedVendors.clear();

	vector<bool> wasVendorFound;
	for(uint i = 0; i < m_vendorNames.size(); i++)
	{
		EXPECT_TRUE(m_ethVendors.IsVendorValid(m_vendorNames[i]));
		if(!m_vendorNames[i].substr(0, 5).compare("Micro"))
		{
			m_expectedVendors.push_back(m_vendorNames[i]);
			wasVendorFound.push_back(false);
		}
	}

	vector<string> foundVendors = m_ethVendors.SearchVendors("Micro");
	for(uint i = 0; i < m_expectedVendors.size(); i++)
	{
		for(uint j = 0; j < foundVendors.size(); j++)
		{
			if(!foundVendors[j].compare(m_expectedVendors[i]))
			{
				wasVendorFound[i] = true;
			}
		}
	}

	for(uint i = 0; i < wasVendorFound.size(); i++)
	{
		EXPECT_TRUE(wasVendorFound[i]);
	}
}

TEST_F(VendorMacDbTest, test_MACGeneration)
{
	string macString = m_ethVendors.GenerateRandomMAC("Dell");
	EXPECT_TRUE(macString.compare(""));
	uint macPrefix = m_ethVendors.AtoMACPrefix(macString);
	EXPECT_TRUE(!m_ethVendors.LookupVendor(macPrefix).compare("Dell"));
}
