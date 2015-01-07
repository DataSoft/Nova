//============================================================================
// Name        : tester_WhitelistConfiguration.h
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
// Description : This file contains unit tests for the class WhitelistConfiguration
//============================================================================

#include "gtest/gtest.h"
#include "WhitelistConfiguration.h"

using namespace Nova;

// The test fixture for testing class WhitelistConfiguration.
class WhitelistConfigurationTest : public ::testing::Test
{

protected:
	WhitelistConfigurationTest()
	{
		chdir(Config::Inst()->GetPathHome().c_str());
	}

};

TEST_F(WhitelistConfigurationTest, test_CRUD)
{
	// Add an IP
	EXPECT_TRUE(WhitelistConfiguration::AddIp("eth0", "3.7.11.13"));
	EXPECT_TRUE(WhitelistConfiguration::AddEntry("eth0,192.168.2.0/24"));

	// Make sure it was added
	vector<string> whitelistedIps = WhitelistConfiguration::GetIps();
	EXPECT_TRUE(find(whitelistedIps.begin(), whitelistedIps.end(), "eth0,3.7.11.13") != whitelistedIps.end());

	vector<string> whitelistedRanges = WhitelistConfiguration::GetIpRanges();
	EXPECT_TRUE(find(whitelistedRanges.begin(), whitelistedRanges.end(), "eth0,192.168.2.0/255.255.255.0") != whitelistedRanges.end());

	// Delete and make sure it was deleted
	EXPECT_TRUE(WhitelistConfiguration::DeleteEntry("eth0,3.7.11.13"));
	whitelistedIps = WhitelistConfiguration::GetIps();
	EXPECT_FALSE(find(whitelistedIps.begin(), whitelistedIps.end(), "eth0,3.7.11.13") != whitelistedIps.end());



}
