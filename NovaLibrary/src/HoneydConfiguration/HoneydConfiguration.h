//============================================================================
// Name        : HoneydConfiguration.h
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
// Description : Object for reading and writing Honeyd XML configurations
//============================================================================

#ifndef _HONEYDCONFIGURATION
#define _HONEYDCONFIGURATION

#include "VendorMacDb.h"
#include "PortSet.h"
#include "ProfileTree.h"
#include "Script.h"
#include "Node.h"

#include <map>
#include <boost/property_tree/ptree.hpp>

#define BOOST_FILESYSTEM_VERSION 2
#include <boost/filesystem.hpp>

namespace Nova
{

class HoneydConfiguration
{

public:

	// This is a singleton class, use this to access it
	//NOTE: Usage of the HoneydConfigurationn object is not threadsafe
	static HoneydConfiguration *Inst();


	//**********************
	//* File IO Operations *
	//**********************

    //Writes out the current HoneydConfiguration object to the Honeyd configuration file in the expected format
    // path: path in the file system to the desired HoneydConfiguration file
    // Returns true if successful and false if not
    bool WriteHoneydConfiguration(std::string path = "");

    //This function takes the current values in the HoneydConfiguration and Config objects
    // 		and translates them into an xml format for persistent storage that can be
    // 		loaded at a later time by any HoneydConfiguration object
    // Returns true if successful and false if the save fails
    bool WriteAllTemplatesToXML();

	//Write the current respective lists out to <template>.xml
    //	If you want to write all of them conveniently, run WriteAllTemplatesToXML()
	//	returns - True on success, false on error
	bool WriteScriptsToXML();
	bool WriteNodesToXML();
	bool WriteProfilesToXML();

    //Populates the HoneydConfiguration object with the xml templates.
    // The configuration is saved and loaded relative to the homepath specified by the Nova Configuration
    // Returns true if successful, false if loading failed.
    bool ReadAllTemplatesXML();

    //Loads respective template from the xml template file located relative to the currently set home path
	// Returns true if successful, false on error
    bool ReadScriptsXML();
    bool ReadNodesXML();
    bool ReadProfilesXML();

	//*****************************
	//* Editing of Configurations *
	//*****************************

	//This function creates a new Honeyd node based on the parameters given
	//	profileName: name of the existing NodeProfile the node should use
	//	ipAddress: string form of the IP address or the string "DHCP" if it should acquire an address using DHCP
	//	macAddress: string form of a MAC address or the string "RANDOM" if one should be generated each time Honeyd is run
	//	interface: the name of the physical or virtual interface the Honeyd node should be deployed on.
	//	portSet: The PortSet to be used for the created node
	//	Returns true if successful and false if not
	bool AddNode(std::string profileName, std::string ipAddress, std::string macAddress,
			std::string interface, int portSetIndex);
	bool AddNode(Node node);

	bool AddNodes(std::string profileName, int portSetIndex, std::string macVendor, std::string ipAddress, std::string interface, int numberOfNodes);


	bool DeletePortSet(std::string profileName, int portSetIndex);
	bool AddPortSet(std::string profileName);


	//Inserts the profile into the honeyd configuration
	//	profile: pointer to the profile you wish to add
	//	Returns (true) if the profile could be created, (false) if it cannot.
	//	NOTE: Gives control of the lifecycle of the provided profile to HoneydConfiguration. Maybe deleting it.
	//			So don't try accessing the profile object after calling this function
	bool AddProfile(Profile *profile);

	bool AddScript(Script script);

	bool AddNewConfiguration(const std::string& configName, bool clone, const std::string& cloneConfig);


	//This function allows access to NodeProfile objects by their name
	// profileName: the name or key of the NodeProfile
	// Returns a pointer to the NodeProfile object or NULL if the key doesn't
	Profile *GetProfile(std::string profileName);

	std::vector<std::string> GetNodeMACs();

	//xxx: Unsafe pointer access into table
	Node *GetNode(std::string nodeMAC);

	//Get a vector of PortSets associated with a particular profile
	std::vector<PortSet*> GetPortSets(std::string profileName);
	PortSet* GetPortSet(std::string profileName, int portSetIndex);

	//This function allows easy access to all profiles
	// Returns a vector of strings containing the names of all profiles
	std::vector<std::string> GetProfileNames();

	Script GetScript(std::string name);
	std::vector<Script> GetScripts(std::string service, std::string osclass = "");

	// Returns a vector of strings containing the names of all non-broadcast scripts
	std::vector<std::string> GetScriptNames();
	
	// Returns a vector of strings containing the names of all broadcast scripts
	std::vector<std::string> GetBroadcastScriptNames();

	//Removes a profile and all associated nodes from the Honeyd configuration
	//	profileName: name of the profile you wish to delete
	// 	Returns: (true) if successful and (false) if the profile could not be found
	bool DeleteProfile(std::string profileName);

	//Deletes a single node
	//	nodeMAC - The MAC address of the node to delete, in string form
	//	returns - True if successfully found and deleted, false otherwise
	bool DeleteNode(std::string nodeMAC);

	bool DeleteScript(std::string name);

	//Iterates through all the ports on all profiles, and removes any instances of the given script
	void DeleteScriptFromPorts(std::string scriptName);


	bool RenameProfile(std::string oldName, std::string newName);

	Profile* GetRoot();

	//Finds out if the given MAC address is in use
	//	mac: the string representation of the MAC address
	//	returns - true if the MAC is in use and false if it is not.
	// *Note this function may have poor performance when there are a large number of nodes
	bool IsMACUsed(std::string mac);


	bool IsIPUsed(std::string ip);

	static std::string SanitizeProfileName(std::string pfilename);

	void ClearProfiles();
	void ClearNodes();

	ProfileTree m_profiles;

	bool SwitchToConfiguration(const std::string&);
	
	bool RemoveConfiguration(const std::string&);
	
	bool LoadConfigurations();
	
	std::vector<std::string> GetConfigurationsList();


	bool SetDoppelganger(Node doppelganger);
	Node GetDoppelganger();

	bool AddScriptOptionValue(std::string scriptName, std::string keyName, std::string value);
	bool DeleteScriptOptionValue(std::string scriptName, std::string keyName, std::string value);

	std::string GenerateRandomUnusedMAC(std::string vendor);

	std::vector<std::string> GetLeafProfileNames();

	//A map structure for node storage
	std::vector<std::string> m_configs;
	NodeTable m_nodes;

private:

    //Basic constructor for the Honeyd Configuration object
	// Initializes the MAC vendor database and hash tables
	// *Note: To populate the object from the file system you must call ReadAllTemplates();
	HoneydConfiguration();

	//Helper function called by WriteProfilesToXML - Writes the profiles out to m_profileTree
	bool WriteProfilesToXML_helper(Profile *root, boost::property_tree::ptree &propTree);

    //Depth first traversal through ptree to read profiles
    Profile *ReadProfilesXML_helper(boost::property_tree::ptree &ptree, Profile *parent);

    void DeleteScriptFromPorts_helper(std::string scriptName, Profile *profile);

    std::vector<std::string> GetLeafProfileNames_helper(Profile *profile);

	static HoneydConfiguration *m_instance;

    //xxx: If we wind up with many scripts, this may not scale well
    std::vector<Script> m_scripts;

    VendorMacDb m_macAddresses;

	// There's only one instance of this node
	Node m_doppelganger;

};

}

#endif
