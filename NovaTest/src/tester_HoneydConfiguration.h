//============================================================================
// Name        : tester_HoneydConfiguration.h
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


#define HC HoneydConfiguration::Inst()

using namespace Nova;

// The test fixture for testing class HoneydConfiguration.
class HoneydConfigurationTest : public ::testing::Test
{

protected:
	// If the constructor and Destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:
	void SetUp()
	{
		EXPECT_TRUE(HC != NULL);
		EXPECT_TRUE(HC->ReadAllTemplatesXML());
		HC->ClearProfiles();
		HC->ClearNodes();
	}
};

TEST_F(HoneydConfigurationTest, test_WriteConfig)
{
	EXPECT_TRUE(HC->WriteHoneydConfiguration(Config::Inst()->GetPathHome()));
}

//need to finish
TEST_F(HoneydConfigurationTest, test_ConfigFunctions)
{
	Node node;
	node.m_MAC= "FF:FF:BA:BE:CA:FE";
	node.m_pfile = "Fedora";
	node.m_enabled = true;
	Profile *defaultProfile = new Profile("default", "DefaultProfile");
	Profile *child1 = new Profile(defaultProfile, "child1");
	Profile *child2 = new Profile(defaultProfile, "child2");
	Profile *child3 = new Profile(child1, "child3, child of child1");
	defaultProfile->SetDropRate("15");
	defaultProfile->SetPersonality("Linux 3.53");
	defaultProfile->SetUptimeMax(400000);
	defaultProfile->SetUptimeMin(0);
	child1->SetDropRate("20");
	child1->SetPersonality("Windows 8");
	child1->SetUptimeMax(100000);
	child1->SetUptimeMin(0);
	child2->SetDropRate("15");
	child2->SetPersonality("Ubuntu 12");
	child2->SetUptimeMax(200000);
	child2->SetUptimeMin(0);
	child3->SetDropRate("13");
	child3->SetPersonality("Fedora");
	child3->SetUptimeMax(300000);
	child3->SetUptimeMin(0);
	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	EXPECT_TRUE(HC->AddProfile(child1));
	EXPECT_TRUE(HC->AddProfile(child2));
	EXPECT_TRUE(HC->AddProfile(child3));
	EXPECT_TRUE(HC->WriteAllTemplatesToXML());
	EXPECT_TRUE(HC->ReadAllTemplatesXML());
	EXPECT_TRUE(HC->ReadNodesXML());
	EXPECT_TRUE(HC->AddNode(node));
	EXPECT_TRUE(HC->WriteHoneydConfiguration());
	EXPECT_TRUE(HC->WriteNodesToXML());
	EXPECT_TRUE(HC->ReadNodesXML());
}

TEST_F(HoneydConfigurationTest, test_WriteAllTemplatesXML)
{
	Profile * defaultProfile = new Profile("default", "DefaultProfile");
	Profile * child1 = new Profile(defaultProfile, "child1");
	Profile * child2 = new Profile(defaultProfile, "child2");
	Profile * child3 = new Profile(child1, "child3, child of child1");
	defaultProfile->SetDropRate("15");
	defaultProfile->SetPersonality("Linux 3.53");
	defaultProfile->SetUptimeMax(400000);
	defaultProfile->SetUptimeMin(0);
	child1->SetDropRate("20");
	child1->SetPersonality("Windows 8");
	child1->SetUptimeMax(100000);
	child1->SetUptimeMin(0);
	child2->SetDropRate("15");
	child2->SetPersonality("Ubuntu 12");
	child2->SetUptimeMax(200000);
	child2->SetUptimeMin(0);
	child3->SetDropRate("13");
	child3->SetPersonality("Fedora");
	child3->SetUptimeMax(300000);
	child3->SetUptimeMin(0);
	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	EXPECT_TRUE(HC->AddProfile(child1));
	EXPECT_TRUE(HC->AddProfile(child2));
	EXPECT_TRUE(HC->AddProfile(child3));
	EXPECT_TRUE(HC->WriteAllTemplatesToXML());
	EXPECT_TRUE(HC->ReadAllTemplatesXML());
	EXPECT_TRUE(HC->GetProfile("DefaultProfile")!=NULL);
	EXPECT_TRUE(HC->GetProfile("DefaultProfile")->GetDropRate().compare("15") == 0);
	EXPECT_TRUE(HC->GetProfile("DefaultProfile")->GetPersonality().compare("Linux 3.53") == 0);
	EXPECT_EQ(400000,HC->GetProfile("DefaultProfile")->GetUptimeMax());
	EXPECT_EQ(0,HC->GetProfile("DefaultProfile")->GetUptimeMin());
	EXPECT_TRUE(HC->GetProfile("child1")!=NULL);
	EXPECT_TRUE(HC->GetProfile("child1")->GetDropRate().compare("20") == 0);
	EXPECT_TRUE(HC->GetProfile("child1")->GetPersonality().compare("Windows 8") == 0);
	EXPECT_EQ(100000,HC->GetProfile("child1")->GetUptimeMax());
	EXPECT_EQ(0,HC->GetProfile("child1")->GetUptimeMin());
	EXPECT_TRUE(HC->GetProfile("child2")!=NULL);
	EXPECT_TRUE(HC->GetProfile("child2")->GetDropRate().compare("15") == 0);
	EXPECT_TRUE(HC->GetProfile("child2")->GetPersonality().compare("Ubuntu 12") == 0);
	EXPECT_EQ(200000,HC->GetProfile("child2")->GetUptimeMax());
	EXPECT_EQ(0,HC->GetProfile("child2")->GetUptimeMin());
	EXPECT_TRUE(HC->GetProfile("child3, child of child1")!=NULL);
	EXPECT_TRUE(HC->GetProfile("child3, child of child1")->GetDropRate().compare("13") == 0);
	EXPECT_TRUE(HC->GetProfile("child3, child of child1")->GetPersonality().compare("Fedora") == 0);
	EXPECT_EQ(300000,HC->GetProfile("child3, child of child1")->GetUptimeMax());
	EXPECT_EQ(0,HC->GetProfile("child3, child of child1")->GetUptimeMin());
	EXPECT_TRUE(HC->GetProfile("DefaultProfile")->m_children[0]->m_name.compare("child1")==0);
	EXPECT_TRUE(HC->GetProfile("DefaultProfile")->m_children[1]->m_name.compare("child2")==0);
	EXPECT_TRUE(HC->GetProfile("child1")->m_children[0]->m_name.compare("child3, child of child1")==0);
}



TEST_F(HoneydConfigurationTest, test_GetRandomVendor)
{
	Profile *defaultProfile = new Profile("default","DefaultProfile");
	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	EXPECT_TRUE(defaultProfile->GetRandomVendor().compare("") > 0);
}

TEST_F(HoneydConfigurationTest, test_GetScript)
{
	Script script;
	script.m_name = "script1";
	EXPECT_TRUE(HC->AddScript(script));
	EXPECT_TRUE(HC->GetScript(script.m_name).m_name.compare(script.m_name) == 0);
	EXPECT_TRUE(HC->DeleteScript(script.m_name));
	EXPECT_TRUE(HC->WriteScriptsToXML());
}
TEST_F(HoneydConfigurationTest, test_GetScripts)
{
	std::vector<Script> receivedScripts;
	Script script1;
	Script script2;
	Script script3;
	Script script4;
	Script script5;
	script1.m_name = "script1";
	script1.m_service = "syslog";
	script2.m_name = "script2";
	script2.m_service = "syslog";
	script3.m_name = "script3";
	script3.m_service = "syslog";
	script4.m_name = "script4";
	script4.m_service = "syslog";
	script5.m_name = "script5";
	EXPECT_TRUE(HC->AddScript(script1));
	EXPECT_TRUE(HC->AddScript(script2));
	EXPECT_TRUE(HC->AddScript(script3));
	EXPECT_TRUE(HC->AddScript(script4));
	EXPECT_TRUE(HC->AddScript(script5));
	receivedScripts = HC->GetScripts("syslog","");
	EXPECT_TRUE(receivedScripts[0].m_service.compare("syslog")==0);
	EXPECT_TRUE(receivedScripts[1].m_service.compare("syslog")==0);
	EXPECT_TRUE(receivedScripts[2].m_service.compare("syslog")==0);
	EXPECT_TRUE(receivedScripts[3].m_service.compare("syslog")==0);
	EXPECT_TRUE(HC->DeleteScript(script1.m_name));
	EXPECT_TRUE(HC->DeleteScript(script2.m_name));
	EXPECT_TRUE(HC->DeleteScript(script3.m_name));
	EXPECT_TRUE(HC->DeleteScript(script4.m_name));
	EXPECT_TRUE(HC->DeleteScript(script5.m_name));
	EXPECT_TRUE(HC->WriteScriptsToXML());
}

TEST_F(HoneydConfigurationTest, test_GetScriptNames)
{
	std::vector<std::string> scriptNames;
	std::vector<string>::iterator it;
	Script script1;
	Script script2;
	Script script3;
	Script script4;
	Script script5;
	script1.m_name = "script1";
	script2.m_name = "script2";
	script3.m_name = "script3";
	script4.m_name = "script4";
	script5.m_name = "script5";
	script1.m_isBroadcastScript = false;
	script2.m_isBroadcastScript = false;
	script3.m_isBroadcastScript = false;
	script4.m_isBroadcastScript = false;
	script5.m_isBroadcastScript = false;
	EXPECT_TRUE(HC->AddScript(script1));
	EXPECT_TRUE(HC->AddScript(script2));
	EXPECT_TRUE(HC->AddScript(script3));
	EXPECT_TRUE(HC->AddScript(script4));
	EXPECT_TRUE(HC->AddScript(script5));
	EXPECT_TRUE(HC->WriteScriptsToXML());
	EXPECT_TRUE(HC->ReadScriptsXML());
	scriptNames = HC->GetScriptNames();
	it = std::find(scriptNames.begin(), scriptNames.end(), "script1");
	EXPECT_TRUE(it->compare("script1")==0);
	it = std::find(scriptNames.begin(), scriptNames.end(), "script2");
	EXPECT_TRUE(it->compare("script2")==0);
	it = std::find(scriptNames.begin(), scriptNames.end(), "script3");
	EXPECT_TRUE(it->compare("script3")==0);
	it = std::find(scriptNames.begin(), scriptNames.end(), "script4");
	EXPECT_TRUE(it->compare("script4")==0);
	EXPECT_TRUE(HC->DeleteScript(script1.m_name));
	EXPECT_TRUE(HC->DeleteScript(script2.m_name));
	EXPECT_TRUE(HC->DeleteScript(script3.m_name));
	EXPECT_TRUE(HC->DeleteScript(script4.m_name));
	EXPECT_TRUE(HC->DeleteScript(script5.m_name));
	EXPECT_TRUE(HC->WriteScriptsToXML());
	EXPECT_TRUE(HC->ReadScriptsXML());
}

TEST_F(HoneydConfigurationTest, test_DeleteProfile)
{
	Profile *defaultProfile = new Profile("default","DefaultProfile");
	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	EXPECT_TRUE(HC->DeleteProfile(defaultProfile->m_name));

}

TEST_F(HoneydConfigurationTest, test_DeleteNode)
{
	Node node;
	EXPECT_TRUE(HC->AddNode(node));
	EXPECT_TRUE(HC->DeleteNode(node.m_MAC));
}

TEST_F(HoneydConfigurationTest, test_DeleteScript)
{
	Script script;
	EXPECT_TRUE(HC->AddScript(script));
	EXPECT_TRUE(HC->DeleteScript(script.m_name));
}

TEST_F(HoneydConfigurationTest, test_IsMACUsed)
{
	string mac = "FF:FF:BA:BE:CA:F2";
	Node node;
	node.m_MAC = "FF:FF:BA:BE:CA:FF";
	EXPECT_TRUE(HC->AddNode(node));
	EXPECT_TRUE(HC->IsMACUsed(node.m_MAC));
	EXPECT_FALSE(HC->IsMACUsed(mac));
}

TEST_F(HoneydConfigurationTest, test_SanitizeProfileName)
{
	Profile *defaultProfile = new Profile("default","defaultProfile , ; @");
	string sanitizedProfileName;
	sanitizedProfileName = HC->SanitizeProfileName(defaultProfile->m_name);
	EXPECT_TRUE(sanitizedProfileName.compare(defaultProfile->m_name)>0);
}

TEST_F(HoneydConfigurationTest, test_ClearProfiles)
{
	Profile *defaultProfile = new Profile("default","defaultProfile , ; @");
	Profile *child1 = new Profile(defaultProfile,"child1");
	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	HC->ClearProfiles();
	EXPECT_TRUE(HC->GetProfile(child1->m_name)==NULL);
}

TEST_F(HoneydConfigurationTest, test_ClearNodes)
{
	Node node;
	node.m_MAC = "FF:AA:BA:BE:CA:F2";
	EXPECT_TRUE(HC->AddNode(node));
	HC->ClearNodes();
	EXPECT_TRUE(HC->GetNode(node.m_MAC) == NULL);
}

TEST_F(HoneydConfigurationTest, test_SwitchToConfiguration)
{
	EXPECT_TRUE(HC->AddNewConfiguration("NewConfig",false,""));
	EXPECT_TRUE(HC->SwitchToConfiguration("NewConfig"));
	EXPECT_TRUE(HC->RemoveConfiguration("NewConfig"));
	EXPECT_TRUE(HC->SwitchToConfiguration("sample"));
}

TEST_F(HoneydConfigurationTest, test_RemoveConfiguration)
{
	std::vector<string>::iterator it;
	std::vector<std::string> ConfigurationList;
	uint configSize = HC->GetConfigurationsList().size();
	EXPECT_TRUE(HC->AddNewConfiguration("NewConfig",false,""));
	ConfigurationList = HC->GetConfigurationsList();
	it = std::find(ConfigurationList.begin(), ConfigurationList.end(), "NewConfig");
	EXPECT_TRUE(ConfigurationList.size()>configSize);
	EXPECT_TRUE(HC->RemoveConfiguration("NewConfig"));
	ConfigurationList = HC->GetConfigurationsList();
	EXPECT_TRUE(ConfigurationList.size()==configSize);
}

TEST_F(HoneydConfigurationTest, test_GetPortSets)
{
	std::vector<PortSet*> vectorOfPorts;
	Profile *defaultProfile = new Profile("default","DefaultProfile");
	defaultProfile->m_portSets.push_back(new PortSet());
	defaultProfile->m_portSets.push_back(new PortSet());
	defaultProfile->m_portSets.push_back(new PortSet());
	defaultProfile->m_portSets.push_back(new PortSet());
	defaultProfile->m_portSets.push_back(new PortSet());
	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	vectorOfPorts = HC->GetPortSets(defaultProfile->m_name);
	EXPECT_TRUE(vectorOfPorts.size() == 5);
}

TEST_F(HoneydConfigurationTest, test_WriteScriptsToXML)
{
	Script script1;
	std::vector<std::string> scriptsInitial;
	std::vector<std::string> scriptsFinal;
	script1.m_name = "script1";
	script1.m_osclass = "Linux";
	script1.m_isBroadcastScript = false;
	script1.m_path	=	Config::Inst()->GetPathHome()+ "/config/templates/default";
	script1.m_service = "N/A";
	scriptsInitial = HC->GetScriptNames();
	EXPECT_TRUE(HC->AddScript(script1));
	EXPECT_TRUE(HC->WriteScriptsToXML());
	EXPECT_TRUE(HC->ReadScriptsXML());
	scriptsFinal = HC->GetScriptNames();
	EXPECT_TRUE(scriptsFinal != scriptsInitial);
	EXPECT_TRUE(HC->DeleteScript(script1.m_name));
	EXPECT_TRUE(HC->WriteScriptsToXML());
	EXPECT_TRUE(HC->ReadScriptsXML());
	scriptsFinal = HC->GetScriptNames();
	EXPECT_TRUE(scriptsInitial == scriptsFinal);
	EXPECT_TRUE(HC->WriteScriptsToXML());
}

TEST_F(HoneydConfigurationTest, test_WriteProfilesToXML)
{
	Profile * defaultProfile = new Profile("default", "DefaultProfile");
	defaultProfile->m_name = "default_Profile_1";
	EXPECT_TRUE(HC->GetProfile(defaultProfile->m_name) == NULL);
	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	EXPECT_TRUE(HC->WriteProfilesToXML());
	EXPECT_TRUE(HC->ReadProfilesXML());
	EXPECT_TRUE(HC->GetProfile(defaultProfile->m_name) != NULL);
	EXPECT_TRUE(HC->DeleteProfile(defaultProfile->m_name));
	EXPECT_TRUE(HC->GetProfile(defaultProfile->m_name) == NULL);
}

TEST_F(HoneydConfigurationTest, test_WriteNodesToXML)
{
	Node node1;
	std::vector<std::string> NodeMACsInitial;
	std::vector<std::string> NodeMACsFinal;
	node1.m_IP = "198.2.3.2.1";
	node1.m_MAC = "FF:FF:BA:BE:CA:FF";
	node1.m_pfile = "";
	NodeMACsInitial = HC->GetNodeMACs();
	EXPECT_TRUE(HC->AddNode(node1));
	EXPECT_TRUE(HC->WriteNodesToXML());
	EXPECT_TRUE(HC->ReadNodesXML());
	NodeMACsFinal = HC->GetNodeMACs();
	EXPECT_TRUE(NodeMACsInitial != NodeMACsFinal);
	EXPECT_TRUE(HC->DeleteNode(node1.m_MAC));
	EXPECT_TRUE(HC->WriteNodesToXML());
	EXPECT_TRUE(HC->ReadNodesXML());
	NodeMACsFinal = HC->GetNodeMACs();
	EXPECT_TRUE(NodeMACsInitial == NodeMACsFinal);
}

TEST_F(HoneydConfigurationTest, test_ProfileWriteDelete)
{
		Profile *profileDefault = new Profile("default", "TestProfile");
		Profile *profileChild1 = new Profile(profileDefault,"Child1");
		Profile *profileChild2 = new Profile(profileDefault,"Child2");
		Profile *profileChild3 = new Profile(profileDefault,"Child3");
		Profile *profileChild4 = new Profile(profileDefault,"Child4");
		Profile *profileChild5 = new Profile(profileChild4,"Child5");
		EXPECT_TRUE(HC->AddProfile(profileDefault));
		EXPECT_TRUE(HC->AddProfile(profileChild1));
		EXPECT_TRUE(HC->AddProfile(profileChild2));
		EXPECT_TRUE(HC->AddProfile(profileChild3));
		EXPECT_TRUE(HC->AddProfile(profileChild4));
		EXPECT_TRUE(HC->AddProfile(profileChild5));
		EXPECT_TRUE(HC->WriteAllTemplatesToXML());
		EXPECT_TRUE(HC->ReadAllTemplatesXML());
		EXPECT_TRUE(HC->GetProfile("Child1")!=NULL);
		EXPECT_TRUE(HC->DeleteProfile("Child1"));
		EXPECT_TRUE(HC->WriteAllTemplatesToXML());
		EXPECT_TRUE(HC->ReadAllTemplatesXML());
		EXPECT_TRUE(HC->GetProfile("Child1")==NULL);
}

TEST_F(HoneydConfigurationTest, test_RenameProfile)
{
	// Create dummy profile
	Profile * p = new Profile("default", "TestProfile");

	// Add the dummy profile
	EXPECT_TRUE(HC->AddProfile(p));
	EXPECT_TRUE(HC->GetProfile("TestProfile") != NULL);

	//Test renaming a profile
	EXPECT_TRUE(HC->RenameProfile("TestProfile", "TestProfile-renamed"));

	// Make sure it was renamed
	EXPECT_TRUE(HC->GetProfile("TestProfile-renamed") != NULL);
	EXPECT_TRUE(HC->GetProfile("TestProfile") == NULL);
}

TEST_F(HoneydConfigurationTest, test_ErrorCases)
{
	EXPECT_FALSE(HC->DeleteProfile(""));
	EXPECT_FALSE(HC->DeleteProfile("aoeustnhaoesnuhaosenuht"));
	EXPECT_FALSE(HC->DeleteNode(""));
	EXPECT_FALSE(HC->DeleteNode("aoeuhaonsehuaonsehu"));
	EXPECT_EQ(NULL, HC->GetProfile(""));
	EXPECT_EQ(NULL, HC->GetProfile("aouhaosnuheaonstuh"));
	EXPECT_EQ(NULL, HC->GetNode(""));
	EXPECT_EQ(NULL, HC->GetNode("aouhaosnuheaonstuh"));

}

TEST_F(HoneydConfigurationTest, test_Profile)
{
	//Create dummy profile
	Profile * p = new Profile("default", "TestProfile");

	//Test adding a profile
	EXPECT_TRUE(HC->AddProfile(p));
	EXPECT_TRUE(HC->GetProfile("TestProfile") != NULL);

	// Add a child profile
	Profile * pChild = new Profile("TestProfile", "TestProfileChild");
	EXPECT_TRUE(HC->AddProfile(pChild));
	EXPECT_TRUE(HC->GetProfile("TestProfileChild") != NULL);

	//Test renaming a profile
	EXPECT_TRUE(HC->RenameProfile("TestProfile", "TestProfileRenamed"));
	EXPECT_TRUE(HC->GetProfile("TestProfile") == NULL);
	EXPECT_TRUE(HC->GetProfile("TestProfileRenamed") != NULL);
	EXPECT_TRUE(HC->GetProfile("TestProfile") == NULL);

	//Test deleting a profile
	EXPECT_TRUE(HC->DeleteProfile("TestProfileRenamed"));
	EXPECT_TRUE(HC->GetProfile("TestProfileRenamed") == NULL);
	EXPECT_TRUE(HC->GetProfile("TestProfileChild") == NULL);
}

TEST_F(HoneydConfigurationTest, test_GetProfileNames)
{
	EXPECT_TRUE(HC->AddProfile(new Profile("default", "top")));
	EXPECT_TRUE(HC->AddProfile(new Profile("default", "top")));
	EXPECT_TRUE(HC->AddProfile(new Profile("top", "topChild")));
	EXPECT_TRUE(HC->AddProfile(new Profile("topChild", "topGrandChild")));

	vector<string> profiles = HC->GetProfileNames();
	// default + 4 new ones (one duplicate) = 4
	EXPECT_EQ(4, profiles.size());
}

TEST_F(HoneydConfigurationTest, test_AddNodes)
{
	EXPECT_TRUE(HC->AddNodes("default", 0, "Dell", "DHCP", "eth0", 10));
	EXPECT_EQ(10, HC->GetNodeMACs().size());
}

TEST_F(HoneydConfigurationTest, test_AddNode)
{
	Node node;
	node.m_MAC = "FF:FF:BA:BE:CA:FE";
	node.m_pfile = "default";

	EXPECT_TRUE(HC->AddNode(node));
	EXPECT_TRUE(HC->GetNode("FF:FF:BA:BE:CA:FE") != NULL);
	EXPECT_TRUE(HC->GetNode("FF:FF:BA:BE:CA:FE")->m_MAC == "FF:FF:BA:BE:CA:FE");
}

TEST_F(HoneydConfigurationTest, test_ReadScriptsXML)
{
	string home = Config::Inst()->GetPathHome() + "/config/templates";
	string command1 = "cd " + home + "; mv scripts.xml script.xml";
	string command2 = "cd " + home + "; mv script.xml scripts.xml";
	int TempNumOne=command1.size();
	char a[100];
	char b[100];
	for (int l=0;l<=TempNumOne;l++)
	        {
	            a[l]=command1[l];
	        }
	TempNumOne = command2.size();
	for (int l=0;l<=TempNumOne;l++)
		        {
		            b[l]=command2[l];
		        }

	EXPECT_TRUE(HC->ReadScriptsXML());
	system(a);
	EXPECT_FALSE(HC->ReadScriptsXML());
	system(b);
	EXPECT_TRUE(HC->ReadScriptsXML());
}

