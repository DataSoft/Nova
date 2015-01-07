//============================================================================
// Name        : tester_Profile.h
// Copyright   : DataSoft Corporation 2011-2012
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
// Description : This file contains unit tests for the class HoneydConfiguration
//============================================================================


#include "gtest/gtest.h"
#include "HoneydConfiguration/HoneydConfiguration.h"
#include "HoneydConfiguration/Profile.h"
#include "HoneydConfiguration/Node.h"
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#define HC HoneydConfiguration::Inst()//this is the instantiation of the honeyd singleton config class

using namespace Nova;

// The test fixture for testing class HoneydConfiguration.
class ProfileTest : public ::testing::Test
{

protected:
	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:
	void SetUp()
	{
		EXPECT_TRUE(HC != NULL);
		EXPECT_TRUE(HC->ReadAllTemplatesXML());
		HC->ClearProfiles();
		HC->ClearNodes();
	}
};

TEST_F(ProfileTest, test_isEquals)
{
	Profile *defaultProfile = new Profile("default","DefaultProfile");
	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	EXPECT_TRUE(defaultProfile->IsEqual(*defaultProfile));
	EXPECT_TRUE(HC->WriteAllTemplatesToXML());
	EXPECT_TRUE(HC->ReadAllTemplatesXML());
}

TEST_F(ProfileTest, test_isEqualsRecursive)
{
	Profile *defaultProfile = new Profile("default","DefaultProfile");
	Profile *child1 = new Profile(defaultProfile,"child1");
	Profile *child2 = new Profile(defaultProfile,"child2");
	Profile *child3 = new Profile(defaultProfile,"child3");
	Profile *child4 = new Profile(child3,"child4");
	Profile *child5 = new Profile(defaultProfile,"child5");
	Profile *child6 = new Profile(defaultProfile,"child6");
	Profile *child7 = new Profile(child1,"child7, child of 1");
	defaultProfile->m_osclass = "linux";
	defaultProfile->SetDropRate("15");
	defaultProfile->m_count = 32;
	defaultProfile->SetPersonality("Lethal");
	defaultProfile->SetUptimeMax(15);
	defaultProfile->SetUptimeMin(0);
	child1->m_osclass = "linux";
	child1->SetDropRate("42");
	child1->m_count = 1;
	child1->SetPersonality("Lethal");
	child1->SetUptimeMax(80000);
	child1->SetUptimeMin(1400);
	child2->m_osclass = "linux";
	child2->SetDropRate("200");
	child2->m_count = 50;
	child2->SetPersonality("Thine enemy");
	child2->SetUptimeMax(15);
	child2->SetUptimeMin(0);
	child3->m_osclass = "FREE BSD";
	child3->SetDropRate("515");
	child3->m_count = 12;
	child3->SetPersonality("thugs");
	child3->SetUptimeMax(15);
	child3->SetUptimeMin(0);
	child4->m_osclass = "Windows";
	child4->SetDropRate("800");
	child4->m_count = 25;
	child4->SetPersonality("threat");
	child4->SetUptimeMax(500);
	child4->SetUptimeMin(0);
	child5->m_osclass = "linux";
	child5->SetDropRate("500");
	child5->m_count = 14;
	child5->SetPersonality("moderate");
	child5->SetUptimeMax(300);
	child5->SetUptimeMin(10);
	child6->m_osclass = "linux";
	child6->SetDropRate("215");
	child6->m_count = 20;
	child6->SetPersonality("Lethal");
	child6->SetUptimeMax(400);
	child6->SetUptimeMin(0);
	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	EXPECT_TRUE(HC->AddProfile(child1));
	EXPECT_TRUE(HC->AddProfile(child2));
	EXPECT_TRUE(HC->AddProfile(child3));
	EXPECT_TRUE(HC->AddProfile(child4));
	EXPECT_TRUE(HC->AddProfile(child5));
	EXPECT_TRUE(HC->AddProfile(child6));
	EXPECT_TRUE(HC->AddProfile(child7));
	EXPECT_TRUE(HC->WriteAllTemplatesToXML());
	EXPECT_TRUE(HC->ReadAllTemplatesXML());
	EXPECT_TRUE(defaultProfile->IsEqualRecursive(*defaultProfile));
}

TEST_F(HoneydConfigurationTest, test_toString)
{
	Profile *defaultProfile = new Profile("default","DefaultProfile");
	Profile *randomProfile = new Profile(defaultProfile,"RandomProfile");
	defaultProfile->m_osclass = "linux";
	defaultProfile->SetDropRate("215");
	defaultProfile->m_count = 20;
	defaultProfile->SetPersonality("Lethal");
	defaultProfile->SetUptimeMax(400);
	defaultProfile->SetUptimeMin(0);
	randomProfile->m_osclass = "Windows Server";
	randomProfile->SetDropRate("15");
	randomProfile->m_count = 5;
	randomProfile->SetPersonality("Threat");
	randomProfile->SetUptimeMax(25);
	randomProfile->SetUptimeMin(0);
	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	EXPECT_TRUE(HC->AddProfile(randomProfile));
	EXPECT_TRUE(defaultProfile->ToString().compare("") > 0);
	EXPECT_TRUE(randomProfile->ToString().compare("") > 0);
}

