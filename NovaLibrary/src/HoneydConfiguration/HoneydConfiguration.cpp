//============================================================================
// Name        : HoneydConfiguration.cpp
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

#include "HoneydConfiguration.h"
#include "../NovaUtil.h"
#include "../Logger.h"

#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <unordered_map>
#include <arpa/inet.h>
#include <math.h>
#include <ctype.h>
#include <netdb.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <algorithm>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sstream>
#include <fstream>

using namespace std;
using namespace Nova;
using boost::property_tree::ptree;
using boost::property_tree::xml_parser::trim_whitespace;

namespace Nova
{

HoneydConfiguration *HoneydConfiguration::m_instance = NULL;

HoneydConfiguration *HoneydConfiguration::Inst()
{
	if(m_instance == NULL)
	{
		m_instance = new HoneydConfiguration();
		m_instance->LoadConfigurations();
		m_instance->ReadAllTemplatesXML();
		m_instance->WriteHoneydConfiguration();
	}
	return m_instance;
}

//Basic constructor for the Honeyd Configuration object
// Initializes the MAC vendor database and hash tables
// *Note: To populate the object from the file system you must call LoadAllTemplates();
HoneydConfiguration::HoneydConfiguration()
{
	m_macAddresses.LoadPrefixFile();
}

bool HoneydConfiguration::ReadAllTemplatesXML()
{
	bool totalSuccess = true;

	if(!ReadScriptsXML())
	{
		totalSuccess = false;
	}
	if(!ReadNodesXML())
	{
		totalSuccess = false;
	}
	if(!ReadProfilesXML())
	{
		totalSuccess = false;
	}

	return totalSuccess;
}

//Loads NodeProfiles from the xml template located relative to the currently set home path
// Returns true if successful, false if not.
bool HoneydConfiguration::ReadProfilesXML()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;
	ptree profilesTopLevel;
	try
	{
		read_xml(Config::Inst()->GetPathHome() + "/config/templates/" + Config::Inst()->GetCurrentConfig() + "/profiles.xml", profilesTopLevel, boost::property_tree::xml_parser::trim_whitespace);

		//Don't loop through the profiles here, as we only expect one root profile. If there are others, ignore them
		ptree rootProfile = profilesTopLevel.get_child("profiles").get_child("profile");
		m_profiles.m_root = ReadProfilesXML_helper(rootProfile, NULL);

		return true;
	}
	catch (boost::property_tree::xml_parser_error &e)
	{
		LOG(ERROR, "Problem loading profiles: " + string(e.what()) + ".", "");
		return false;
	}
	catch (boost::property_tree::ptree_error &e)
	{
		LOG(ERROR, "Problem loading profiles: " + string(e.what()) + ".", "");
		return false;
	}
	return false;
}

Profile *HoneydConfiguration::ReadProfilesXML_helper(ptree &ptree, Profile *parent)
{
	Profile *profile = NULL;

	try
	{
		profile = new Profile(parent, ptree.get<string>("name"));
		profile->m_count = ptree.get<double>("count");
		profile->SetPersonality(ptree.get<string>("personality"));
		profile->SetUptimeMin(ptree.get<uint>("uptimeMin"));
		profile->SetUptimeMax(ptree.get<uint>("uptimeMax"));
		profile->SetDropRate(ptree.get<string>("dropRate"));
		profile->m_isPersonalityInherited = ptree.get<bool>("isPersonalityInherited");
		profile->m_isUptimeInherited = ptree.get<bool>("isUptimeInherited");
		profile->m_isDropRateInherited = ptree.get<bool>("isDropRateInherited");

		//Ethernet Settings
		try
		{
			BOOST_FOREACH(ptree::value_type &ethernetVendors, ptree.get_child("ethernet_vendors"))
			{
				if(!ethernetVendors.first.compare("vendor"))
				{
					string vendorName = ethernetVendors.second.get<string>("prefix");
					uint count = ethernetVendors.second.get<uint>("count");

					profile->m_vendors.push_back(pair<string, uint>(vendorName, count));
				}
			}
		}
		catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
		{
			LOG(DEBUG, "Unable to parse Ethernet settings for profile", "");
		};

		// Broadcast scripts
		try
		{
			BOOST_FOREACH(ptree::value_type &broadcasts, ptree.get_child("broadcasts"))
			{
				if(!string(broadcasts.first.data()).compare("broadcast"))
				{
					Broadcast *bcast = new Broadcast();
					bcast->m_script = broadcasts.second.get<string>("script");
					bcast->m_dstPort = broadcasts.second.get<int>("dstport");
					bcast->m_srcPort = broadcasts.second.get<int>("srcport");
					bcast->m_time = broadcasts.second.get<int>("time");

					profile->m_broadcasts.push_back(bcast);
				}
			}
		}
		catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
		{
			LOG(DEBUG, "Unable to parse Broacdast settings for profile:" + string(e.what()), "");
		};

		// Proxy services
		try
		{
			BOOST_FOREACH(ptree::value_type &proxies, ptree.get_child("proxies"))
			{
				if(!string(proxies.first.data()).compare("proxy"))
				{
					Proxy *proxy = new Proxy();
					proxy->m_honeypotPort = proxies.second.get<int>("honeypotport");
					proxy->m_proxyIP = proxies.second.get<string>("proxyip");
					proxy->m_protocol = proxies.second.get<string>("protocol");
					proxy->m_proxyPort = proxies.second.get<int>("proxyport");

					profile->m_proxies.push_back(proxy);
				}
			}
		}
		catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
		{
			LOG(DEBUG, "Unable to parse proxy service settings for profile:" + string(e.what()), "");
		};



		//Port Sets
		try
		{
			BOOST_FOREACH(ptree::value_type &portsets, ptree.get_child("portsets"))
			{
				if(!string(portsets.first.data()).compare("portset"))
				{
					PortSet *portSet = new PortSet();

					portSet->m_defaultTCPBehavior = Port::StringToPortBehavior(portsets.second.get<string>("defaultTCPBehavior"));
					portSet->m_defaultUDPBehavior = Port::StringToPortBehavior(portsets.second.get<string>("defaultUDPBehavior"));
					portSet->m_defaultICMPBehavior = Port::StringToPortBehavior(portsets.second.get<string>("defaultICMPBehavior"));

					//Exceptions
					BOOST_FOREACH(ptree::value_type &ports, portsets.second.get_child("exceptions"))
					{
						Port port;

						port.m_service = ports.second.get<string>("service");
						port.m_scriptName = ports.second.get<string>("script");
						port.m_portNumber = ports.second.get<uint>("number");
						port.m_behavior = Port::StringToPortBehavior(ports.second.get<string>("behavior"));
						port.m_protocol = Port::StringToPortProtocol(ports.second.get<string>("protocol"));

						for (ptree::const_iterator it = ports.second.begin(); it != ports.second.end(); ++it)
						{
							if (it->first == "option")
							{
								string key = it->second.get<string>("key");
								string value = it->second.get<string>("value");

								port.m_scriptConfiguration[key] = value;
							}
						}


						portSet->AddPort(port);
					}

					profile->m_portSets.push_back(portSet);
				}
			}
		}
		catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
		{
			LOG(DEBUG, "Unable to parse PortSet settings for profile:" + string(e.what()), "");
		};

		//Recursively add children
		try
		{
			BOOST_FOREACH(ptree::value_type &children, ptree.get_child("profiles"))
			{
				Profile *child = ReadProfilesXML_helper(children.second, profile);
				if(child != NULL)
				{
					profile->m_children.push_back(child);
				}
			}
			//Calculate the distributions for the children nodes
			profile->RecalculateChildDistributions();

		}
		catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
		{
		};
	}
	catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
	{
		LOG(WARNING, "Unable to parse required values for the NodeProfiles!", "");
	};

	return profile;
}

void HoneydConfiguration::DeleteScriptFromPorts_helper(string scriptName, Profile *profile)
{
	if(profile == NULL)
	{
		return;
	}

	//Depth first traversal of tree
	for(uint i = 0; i < profile->m_children.size(); i++)
	{
		DeleteScriptFromPorts_helper(scriptName, profile->m_children[i]);
	}

	//Search through all the port sets
	for(uint i = 0; i < profile->m_portSets.size(); i++)
	{
		//And all the ports inside the port set
		for(uint j = 0; j < profile->m_portSets[i]->m_portExceptions.size(); j++)
		{
			if(profile->m_portSets[i]->m_portExceptions[j].m_scriptName == scriptName)
			{
				profile->m_portSets[i]->m_portExceptions[j].m_behavior = PORT_OPEN;
				profile->m_portSets[i]->m_portExceptions[j].m_scriptName = "";
			}
		}
	}
}

void HoneydConfiguration::DeleteScriptFromPorts(std::string scriptName)
{
	DeleteScriptFromPorts_helper(scriptName, m_profiles.m_root);
}

//Loads Nodes from the xml template located relative to the currently set home path
// Returns true if successful, false if not.
bool HoneydConfiguration::ReadNodesXML()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;

	ptree propTree;

	m_nodes.clear();

	try
	{
		read_xml(Config::Inst()->GetPathHome() + "/config/templates/" + Config::Inst()->GetCurrentConfig() + "/nodes.xml", propTree, boost::property_tree::xml_parser::trim_whitespace);


		//For each node tag
		try
		{
			BOOST_FOREACH(ptree::value_type &nodePtree, propTree.get_child("nodes"))
			{
				if(!nodePtree.first.compare("node"))
				{
					Node node(nodePtree.second);
					AddNode(node);
				}
			}
		}
		catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
		{
			LOG(DEBUG, "Unable to parse a node in nodes.xml: " + string(e.what()), "");
		};

		//For the doppelganger tag
		try
		{
			int doppelcount = 0;
			BOOST_FOREACH(ptree::value_type &nodePtree, propTree.get_child("doppelganger"))
			{
				if(!nodePtree.first.compare("node"))
				{
					doppelcount++;
					if (doppelcount != 1) {
						LOG(WARNING, "XML appears to contain more than one doppelganger node!", "");
					}

					Node node(nodePtree.second);
					m_doppelganger = node;
				}
			}
		}
		catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
		{
			LOG(DEBUG, "Unable to parse a node in nodes.xml: " + string(e.what()), "");
		};

	}
	catch(Nova::hashMapException &e)
	{
		LOG(ERROR, "Problem loading nodes: " + string(e.what()) + ".", "");
		return false;
	}
	catch (boost::property_tree::xml_parser_error &e) {
		LOG(ERROR, "Problem loading nodes: " + string(e.what()) + ".", "");
		return false;
	}
	catch (boost::property_tree::ptree_error &e)
	{
		LOG(ERROR, "Problem loading nodes: " + string(e.what()) + ".", "");
		return false;
	}
	return true;
}

void print(boost::property_tree::ptree const& pt)
{
    using boost::property_tree::ptree;
    ptree::const_iterator end = pt.end();
    for (ptree::const_iterator it = pt.begin(); it != end; ++it) {
        std::cout << it->first << ": " << it->second.get_value<std::string>() << std::endl;
        //print(it->second);
    }
}


//Loads scripts from the xml template located relative to the currently set home path
// Returns true if successful, false if not.
bool HoneydConfiguration::ReadScriptsXML()//write complex test that moves the xml scripts to different locations b4 attempting to read
{//
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;
	ptree scriptsTopLevel;
	m_scripts.clear();
	try
	{
		read_xml(Config::Inst()->GetPathHome() + "/config/templates/scripts.xml", scriptsTopLevel, boost::property_tree::xml_parser::trim_whitespace);

		BOOST_FOREACH(ptree::value_type &value, scriptsTopLevel.get_child("scripts"))
		{
			Script script;
			try
			{
				//Each script consists of a name and path to that script
				script.m_name = value.second.get<string>("name");

				if(script.m_name == "")
				{
					LOG(DEBUG, "Read a Invalid name for script", "");
					continue;
				}

				script.m_service = value.second.get<string>("service");
				script.m_osclass = value.second.get<string>("osclass");
				script.m_path = value.second.get<string>("path");
				script.m_defaultPort = value.second.get<string>("defaultport");
				script.m_defaultProtocol = value.second.get<string>("defaultprotocol");
				script.m_isBroadcastScript = value.second.get<bool>("broadcast");
				script.m_isConfigurable = value.second.get<bool>("configurable");

				//cout << "Configurable is " << script.m_isConfigurable << endl;

				if (script.m_isConfigurable)
				{
					for (ptree::const_iterator it = value.second.begin(); it != value.second.end(); ++it)
					{
						if (it->first == "option")
						{
							vector<string> possibleValues;
							string key = it->second.get<string>("key");
							script.optionDescriptions[key] = "";
							//cout << "Key is " << key << endl;

							for (ptree::const_iterator valIt = it->second.begin(); valIt != it->second.end(); ++valIt) {
								if (valIt->first == "value")
								{
									possibleValues.push_back(valIt->second.data());
									//cout << "value is " << possibleValues.at(possibleValues.size() - 1) << endl;
								}
								else if(valIt->first == "description")
								{
									script.optionDescriptions[key] = valIt->second.data();
								}
							}

							script.options[key] = possibleValues;
						}
					}
				}

				AddScript(script);
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
			{
				LOG(DEBUG, "Could not read script '" + script.m_name + "'" + " due to error " + e.what(), "");
			};
		}
	}
	catch(Nova::hashMapException &e)
	{
		LOG(WARNING, "Problem loading scripts: " + string(e.what()) + ".", "");
		return false;
	}
	catch (boost::property_tree::xml_parser_error &e)
	{
		LOG(WARNING, "Problem loading scripts: " + string(e.what()) + ".", "");
		return false;
	}
	catch (boost::property_tree::ptree_error &e)
	{
		LOG(WARNING, "Problem loading scripts: " + string(e.what()) + ".", "");
		return false;
	}
	return true;
}

bool HoneydConfiguration::WriteScriptsToXML()
{
	ptree scriptsTopLevel;

	for(uint i = 0; i < m_scripts.size(); i++)
	{
		ptree propTree;
		propTree.put<string>("name", m_scripts[i].m_name);
		propTree.put<string>("service", m_scripts[i].m_service);
		propTree.put<string>("osclass", m_scripts[i].m_osclass);
		propTree.put<string>("path", m_scripts[i].m_path);
		propTree.put<string>("defaultport", m_scripts[i].m_defaultPort);
		propTree.put<string>("defaultprotocol", m_scripts[i].m_defaultProtocol);
		propTree.put<bool>("broadcast", m_scripts[i].m_isBroadcastScript);
		propTree.put<bool>("configurable", m_scripts[i].m_isConfigurable);

		for (std::map<std::string, std::vector<std::string>>::iterator it = m_scripts[i].options.begin(); it != m_scripts[i].options.end(); it++)
		{
			ptree optionTree;
			optionTree.put<string>("key", it->first);
			for(uint i = 0; i < it->second.size(); i++)
			{
				optionTree.add<string>("value", it->second[i]);
			}

			propTree.add_child("option", optionTree);
		}
		scriptsTopLevel.add_child("scripts.script", propTree);
	}

	try
	{
		boost::property_tree::xml_writer_settings<char> settings('\t', 1);
		string homePath = Config::Inst()->GetPathHome();
		write_xml(homePath + "/config/templates/scripts.xml", scriptsTopLevel, locale(), settings);
	}
	catch(boost::property_tree::xml_parser_error &e)
	{
		LOG(ERROR, "Unable to write to xml files, caught exception " + string(e.what()), "");
		return false;
	}
	return true;
}

bool HoneydConfiguration::WriteNodesToXML()
{
	ptree nodesTopLevel;

	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		nodesTopLevel.add_child("nodes.node", it->second.GetPtree());
	}
	nodesTopLevel.add_child("doppelganger.node", m_doppelganger.GetPtree());

	//Actually write out to file
	try
	{
		boost::property_tree::xml_writer_settings<char> settings('\t', 1);
		string homePath = Config::Inst()->GetPathHome();
		write_xml(homePath + "/config/templates/" + Config::Inst()->GetCurrentConfig() + "/nodes.xml", nodesTopLevel, locale(), settings);
	}
	catch(boost::property_tree::xml_parser_error &e)
	{
		LOG(ERROR, "Unable to write to xml files, caught exception " + string(e.what()), "");
		return false;
	}
	return true;
}

bool HoneydConfiguration::WriteProfilesToXML()
{
	ptree profilesToplevel;
	ptree rootTree;

	if(WriteProfilesToXML_helper(m_profiles.m_root, rootTree))
	{
		try
		{
			profilesToplevel.add_child("profiles.profile", rootTree);

			boost::property_tree::xml_writer_settings<char> settings('\t', 1);
			string homePath = Config::Inst()->GetPathHome();
			write_xml(homePath + "/config/templates/" + Config::Inst()->GetCurrentConfig() + "/profiles.xml", profilesToplevel, locale(), settings);
		}
		catch(boost::property_tree::xml_parser_error &e)
		{
			LOG(ERROR, "Unable to write to xml files, caught exception " + string(e.what()), "");
			return false;
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool HoneydConfiguration::WriteProfilesToXML_helper(Profile *root, ptree &propTree)
{
	if(root == NULL)
	{
		return false;
	}

	//Write the current item into the tree
	propTree.put<string>("name", root->m_name);
	propTree.put<double>("count", root->m_count);
	propTree.put<string>("personality", root->GetPersonalityNonRecursive());
	propTree.put<uint>("uptimeMin", root->GetUptimeMinNonRecursive());
	propTree.put<uint>("uptimeMax", root->GetUptimeMaxNonRecursive());
	propTree.put<string>("dropRate", root->GetDropRateNonRecursive());
	propTree.put<bool>("isPersonalityInherited", root->m_isPersonalityInherited);
	propTree.put<bool>("isUptimeInherited", root->m_isUptimeInherited);
	propTree.put<bool>("isDropRateInherited", root->m_isDropRateInherited);

	//Ethernet settings
	ptree vendors;
	for(uint i = 0; i < root->m_vendors.size(); i++)
	{
		ptree vendor;

		vendor.put<string>("prefix", root->m_vendors[i].first);
		vendor.put<uint>("count", root->m_vendors[i].second);

		vendors.add_child("vendor", vendor);
	}
	propTree.add_child("ethernet_vendors", vendors);


	ptree broadcasts;
	for (uint i = 0; i < root->m_broadcasts.size(); i++)
	{
		ptree broadcast;
		broadcast.put<std::string>("script", root->m_broadcasts[i]->m_script);
		broadcast.put<int>("srcport", root->m_broadcasts[i]->m_srcPort);
		broadcast.put<int>("dstport", root->m_broadcasts[i]->m_dstPort);
		broadcast.put<int>("time", root->m_broadcasts[i]->m_time);

		broadcasts.add_child("broadcast", broadcast);
	}

	propTree.add_child("broadcasts",broadcasts);

	ptree proxies;
	for (uint i = 0; i < root->m_proxies.size(); i++)
	{
		ptree proxy;
		proxy.put<int>("honeypotport", root->m_proxies[i]->m_honeypotPort);
		proxy.put<string>("protocol", root->m_proxies[i]->m_protocol);
		proxy.put<string>("proxyip", root->m_proxies[i]->m_proxyIP);
		proxy.put<int>("proxyport", root->m_proxies[i]->m_proxyPort);

		proxies.add_child("proxy", proxy);
	}

	propTree.add_child("proxies",proxies);

	//Describe what port sets are available for this profile
	{
		ptree portSets;

		for(uint i = 0; i < root->m_portSets.size(); i++)
		{
			ptree portSet;

			portSet.put<string>("defaultTCPBehavior", Port::PortBehaviorToString(root->m_portSets[i]->m_defaultTCPBehavior));
			portSet.put<string>("defaultUDPBehavior", Port::PortBehaviorToString(root->m_portSets[i]->m_defaultUDPBehavior));
			portSet.put<string>("defaultICMPBehavior", Port::PortBehaviorToString(root->m_portSets[i]->m_defaultICMPBehavior));

			//A new subtree for the exceptions
			ptree exceptions;
			//Foreach exception
			for(uint j = 0; j < root->m_portSets[i]->m_portExceptions.size(); j++)
			{
				//Make a sub-tree for this Port
				ptree port;

				port.put<string>("service", root->m_portSets[i]->m_portExceptions[j].m_service);
				port.put<string>("script", root->m_portSets[i]->m_portExceptions[j].m_scriptName);
				port.put<uint>("number", root->m_portSets[i]->m_portExceptions[j].m_portNumber);
				port.put<string>("behavior", Port::PortBehaviorToString(root->m_portSets[i]->m_portExceptions[j].m_behavior));
				port.put<string>("protocol", Port::PortProtocolToString(root->m_portSets[i]->m_portExceptions[j].m_protocol));

				for (map<string,string>::iterator it = root->m_portSets[i]->m_portExceptions[j].m_scriptConfiguration.begin(); it != root->m_portSets[i]->m_portExceptions[j].m_scriptConfiguration.end(); it++)
				{
					ptree option;
					option.put<string>("key", it->first);
					option.put<string>("value", it->second);
					port.add_child("option", option);
				}

				exceptions.add_child("port", port);
			}

			portSet.add_child("exceptions", exceptions);

			portSets.add_child("portset", portSet);
		}

		propTree.add_child("portsets",portSets);
	}

	ptree children;

	//Then write all of its children
	for(uint i = 0; i < root->m_children.size(); i++)
	{
		ptree child;
		WriteProfilesToXML_helper(root->m_children[i], child);
		children.add_child("profile", child);
	}

	if(!root->m_children.empty())
	{
		propTree.add_child("profiles", children);
	}

	return true;
}

bool HoneydConfiguration::WriteAllTemplatesToXML()
{
	bool totalSuccess = true;

	if(!WriteScriptsToXML())
	{
		totalSuccess = false;
	}
	if(!WriteNodesToXML())
	{
		totalSuccess = false;
	}
	if(!WriteProfilesToXML())
	{
		totalSuccess = false;
	}

	return totalSuccess;
}

bool HoneydConfiguration::WriteHoneydConfiguration(string path)
{
	if(!path.compare(""))
	{
		if(!Config::Inst()->GetPathConfigHoneydHS().compare(""))
		{
			LOG(ERROR, "Invalid path given to Honeyd configuration file!", "");
			return false;
		}
		path = Config::Inst()->GetPathHome() + "/" + Config::Inst()->GetPathConfigHoneydHS();
	}

	stringstream out;

	out << m_profiles.m_root->ToString() << "\n";


	// Shuffle the nodes to avoid having DHCP requests that end up contiguously grouped by profile
	vector<string> shuffledKeys;
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		shuffledKeys.push_back(it->first);
	}
	std::random_shuffle(shuffledKeys.begin(), shuffledKeys.end());


	for (uint j = 0; j < shuffledKeys.size(); j++)
	{
		if(!m_nodes[shuffledKeys[j]].m_enabled)
		{
			continue;
		}

		stringstream ss;
		ss << "CustomNodeProfile-" << j;
		string nodeName = ss.str();

		Profile *item = m_profiles.GetProfile(m_nodes[shuffledKeys[j]].m_pfile);
		if(item != NULL)
		{
			//Print the profile
			out << item->ToString(m_nodes[shuffledKeys[j]].m_portSetIndex, nodeName);
			//Then we need to add node-specific information to the profile's output
			if(!m_nodes[shuffledKeys[j]].m_IP.compare("DHCP"))
			{
				out << "dhcp " << nodeName << " on " << m_nodes[shuffledKeys[j]].m_interface;
				//If the node has a MAC address (not random generated)
				if(m_nodes[shuffledKeys[j]].m_MAC.compare("RANDOM"))
				{
					out << " ethernet \"" << m_nodes[shuffledKeys[j]].m_MAC << "\"";
				}
				out << "\n\n";
			}
			else
			{
				//If the node has a MAC address (not random generated)
				if(m_nodes[shuffledKeys[j]].m_MAC.compare("RANDOM"))
				{
					//Set the MAC for the custom node profile
					out << "set " << nodeName << " ethernet \"" << m_nodes[shuffledKeys[j]].m_MAC << "\"" << '\n';
				}
				//bind the node to the IP address
				out << "bind " << m_nodes[shuffledKeys[j]].m_IP << " " << nodeName << "\n\n";
			}
		}
	}

	ofstream outFile(path);
	outFile << out.str() << '\n';
	outFile.close();
	return true;
}

bool HoneydConfiguration::AddNode(string profileName, string ipAddress, string macAddress,
		string interface, int portSetIndex)
{
	Node newNode;
	uint macPrefix = m_macAddresses.AtoMACPrefix(macAddress);
	string vendor = m_macAddresses.LookupVendor(macPrefix);

	//Finish populating the node
	newNode.m_interface = interface;
	newNode.m_pfile = profileName;
	newNode.m_enabled = true;
	newNode.m_portSetIndex = portSetIndex;

	//Check the IP  and MAC address
	if(ipAddress.compare("DHCP"))
	{
		//Lookup the mac vendor to assert a valid mac
		if(!m_macAddresses.IsVendorValid(vendor))
		{
			LOG(WARNING, "Invalid MAC string '" + macAddress + "' given!", "");
		}

		uint retVal = inet_addr(ipAddress.c_str());
		if(retVal == INADDR_NONE)
		{
			LOG(WARNING, "Invalid node IP address '" + ipAddress + "' given!", "");
			return false;
		}

		// Make sure the IP address hasn't been used already
		if (IsIPUsed(ipAddress))
		{
			LOG(DEBUG, "Attempt at creation of a node using IP address that already exists", "");
			return false;
		}

	}

	//Get the name after assigning the values
	newNode.m_MAC = macAddress;
	newNode.m_IP = ipAddress;

	Profile *profile = GetProfile(profileName);
	if(profile == NULL)
	{
		return false;
	}

	if(m_nodes.keyExists(newNode.m_MAC))
	{
		return false;
	}
	else
	{
		m_nodes[newNode.m_MAC] = newNode;
		return true;
	}
}

bool HoneydConfiguration::AddNode(Node node)
{
	if(m_nodes.keyExists(node.m_MAC))
	{
		return false;
	}
	else
	{
		m_nodes[node.m_MAC] = node;
		return true;
	}
}

bool HoneydConfiguration::AddNodes(string profileName, int portSetIndex, string macVendor, string ipAddress, string interface, int numberOfNodes)
{
	Profile *profile = GetProfile(profileName);
	if(profile == NULL)
	{
		LOG(DEBUG, "Unable to find valid profile named '" + profileName + "' during node creation!", "");
		return false;
	}

	if(numberOfNodes <= 0)
	{
		LOG(DEBUG, "Must create 1 or more nodes", "");
		return false;
	}

	//Add nodes in the DHCP case
	if(!ipAddress.compare("DHCP"))
	{
		for(int i = 0; i < numberOfNodes; i++)
		{
			string macAddress = m_macAddresses.GenerateRandomMAC(macVendor);
			if(!AddNode(profileName, ipAddress, macAddress, interface, portSetIndex))
			{
				LOG(WARNING, "Adding new nodes failed during node creation!", "");
				return false;
			}
		}
		return true;
	}

	//Check the starting ipaddress
	in_addr_t sAddr = inet_addr(ipAddress.c_str());
	if(sAddr == INADDR_NONE)
	{
		LOG(WARNING,"Invalid IP Address given!", "");
	}

	//Add nodes in the statically addressed case
	sAddr = ntohl(sAddr);
	//Removes un-init compiler warning given for in_addr currentAddr;
	in_addr currentAddr = *(in_addr *)&sAddr;

	for(int i = 0; i < numberOfNodes; i++)
	{
		currentAddr.s_addr = htonl(sAddr);
		string macAddress = m_macAddresses.GenerateRandomMAC(macVendor);
		if(!AddNode(profileName, string(inet_ntoa(currentAddr)), macAddress, interface, portSetIndex))
		{
			LOG(ERROR, "Adding new nodes failed during node creation!", "");
			return false;
		}
		sAddr++;
	}
	return true;
}

//Recursive helper function to GetProfileNames()
vector<string> GetProfileNames_helper(Profile *item)
{
	if(item == NULL)
	{
		//Return an empty vector
		return vector<string>();
	}

	vector<string> runningTotalProfiles;

	//Only add this profile's name if we're not only looking for generated profiles, or if it is generated anyway
	runningTotalProfiles.push_back(item->m_name);

	//Depth first traversal of tree
	for(uint i = 0; i < item->m_children.size(); i++)
	{
		vector<string> childProfiles = GetProfileNames_helper(item->m_children[i]);

		runningTotalProfiles.insert(runningTotalProfiles.end(), childProfiles.begin(), childProfiles.end());
	}

	return runningTotalProfiles;
}

vector<string> HoneydConfiguration::GetProfileNames()
{
	return GetProfileNames_helper(m_profiles.m_root);
}


vector<string> HoneydConfiguration::GetScriptNames()
{
	vector<string> scriptNames;
	for(uint i = 0; i < m_scripts.size(); i++)
	{
		if (!m_scripts[i].m_isBroadcastScript)
		{
			scriptNames.push_back(m_scripts[i].m_name);
		}
	}
	return scriptNames;
}

vector<string> HoneydConfiguration::GetBroadcastScriptNames()
{
	vector<string> scriptNames;
	for(uint i = 0; i < m_scripts.size(); i++)
	{
		if (m_scripts[i].m_isBroadcastScript)
		{
			scriptNames.push_back(m_scripts[i].m_name);
		}
	}
	return scriptNames;
}

Profile *GetProfile_helper(string profileName, Profile *item)
{
	if(item == NULL)
	{
		return NULL;
	}

	if(item->m_name == profileName)
	{
		return item;
	}

	for(uint i = 0; i < item->m_children.size(); i++)
	{
		Profile *profile =  GetProfile_helper(profileName, item->m_children[i]);
		if(profile != NULL)
		{
			return profile;
		}
	}

	return NULL;
}

Profile *HoneydConfiguration::GetProfile(string profileName)
{
	return GetProfile_helper(profileName, m_profiles.m_root);
}

// *Note this function may have poor performance when there are a large number of nodes
bool HoneydConfiguration::IsMACUsed(string mac)
{
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		if(!it->first.compare(mac))
		{
			return true;
		}
	}

	return false;
}

bool HoneydConfiguration::IsIPUsed(string ip)
{
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		if(it->second.m_IP == ip)
		{
			return true;
		}
	}

	return false;

}


string HoneydConfiguration::GenerateRandomUnusedMAC(string vendor)
{
	string mac;

	do
	{
		mac = m_macAddresses.GenerateRandomMAC(vendor);
	} while (IsMACUsed(mac));

	return mac;
}


bool HoneydConfiguration::AddProfile(Profile *profile)
{
	if(profile == NULL)
	{
		return false;
	}

	//Check to see if the profile name already exists
	Profile *duplicate = GetProfile(profile->m_name);
	if(duplicate != NULL)
	{
		//Copy over the contents of this profile, and quit
		duplicate->Copy(profile);
	
		//We don't need this new profile anymore, so get rid of it
		if(profile != NULL)
		{
			delete profile;
		}
		return true;
	}

	if(profile->m_parent == NULL)
	{
		//You can't replace the root profile "default"
		return false;
	}

	Profile *parent = GetProfile(profile->m_parent->m_name);
	if(parent == NULL)
	{
		//Parent didn't exist
		return false;
	}

	//Add the new profile to its parent's list of children
	parent->m_children.push_back(profile);
	//Add the parent as the current profile's parent
	profile->m_parent = parent;

	return true;
}

vector<string> HoneydConfiguration::GetNodeMACs()
{
	vector<string> MACs;

	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		MACs.push_back(it->first);
	}

	return MACs;
}

bool HoneydConfiguration::RenameProfile(string oldName, string newName)
{
	Profile *profile = GetProfile(oldName);
	if(profile == NULL)
	{
		return false;
	}

	profile->m_name = newName;
	return true;
}

bool HoneydConfiguration::DeleteNode(string nodeMAC)
{
	if(m_nodes.keyExists(nodeMAC))
	{
		m_nodes.erase(nodeMAC);
		return true;
	}

	return false;
}

Node *HoneydConfiguration::GetNode(string nodeMAC)
{
	if(m_nodes.keyExists(nodeMAC))
	{
		//XXX: Unsafe address access into table
		return &m_nodes[nodeMAC];
	}

	return NULL;
}

std::vector<PortSet*> HoneydConfiguration::GetPortSets(std::string profileName)
{
	vector<PortSet*> portSets;

	Profile *profile = GetProfile(profileName);
	if(profile == NULL)
	{
		return portSets;
	}

	return profile->m_portSets;
}

PortSet* HoneydConfiguration::GetPortSet(string profileName, int portSetIndex)
{
	vector<PortSet*> PortSets;
	PortSets = GetPortSets(profileName);

	if (portSetIndex < 0 || portSetIndex >= (int)PortSets.size())
	{
		return NULL;
	}

	return PortSets[portSetIndex];
}

bool HoneydConfiguration::AddPortSet(string profileName)
{
	Profile *profile = GetProfile(profileName);
	if(profile == NULL)
	{
		LOG(DEBUG, "Attempt to delete portset of profile that doesn't exist: " + profileName, "");
		return false;
	}

	profile->m_portSets.push_back(new PortSet());

	return true;
}

bool HoneydConfiguration::DeletePortSet(string profileName, int portSetIndex)
{
	Profile *profile = GetProfile(profileName);
	if(profile == NULL)
	{
		LOG(DEBUG, "Attempt to delete portset of profile that doesn't exist: " + profileName, "");
		return false;
	}

	// If the index doesn't exist
	if (portSetIndex < 0 || portSetIndex >= (int)profile->m_portSets.size())
	{
		LOG(DEBUG, "Attempt to delete invalid portset index of profile " + profileName, "");
		return false;
	}

	// Delete any nodes that use this portset
	NodeTable::iterator it = m_nodes.begin();
	while (it != m_nodes.end())
	{
		if(it->second.m_pfile == profileName && it->second.m_portSetIndex == portSetIndex)
		{
			// Note: you need to increment it before deleting it,
			// since it becomes invalidated once erased. it++ increments
			// it but returns the original iterrator value for .erase();
			m_nodes.erase(it++);
		}
		else
		{
			++it;
		}
	}

	// If the doppelganger uses this portset, change it to default
	if (m_doppelganger.m_pfile == profile->m_name && m_doppelganger.m_portSetIndex == portSetIndex)
	{
		m_doppelganger.m_portSetIndex = 0;
	}


	// We have to adjust nodes so they point to the correct portsets that had index changes
	if (portSetIndex != (int)(profile->m_portSets.size() - 1))
	{
		NodeTable::iterator it = m_nodes.begin();
		while (it != m_nodes.end())
		{
			if(it->second.m_portSetIndex > portSetIndex)
			{
				// Because all of our indexes shifted with the delete, decrement the index
				it->second.m_portSetIndex--;
			}

			++it;
		}
	}

	delete profile->m_portSets[portSetIndex];
	profile->m_portSets.erase(profile->m_portSets.begin() + portSetIndex);

	return true;
}

bool HoneydConfiguration::DeleteProfile(string profileName)
{
	Profile *profile = GetProfile(profileName);
	if(profile == NULL)
	{
		return false;
	}

	// You're not allowed to delete the root
	if (profile->m_parent == NULL)
	{
		return false;
	}

	// Delete any nodes that use this profile
	NodeTable::iterator it = m_nodes.begin();
	while (it != m_nodes.end())
	{
		if(it->second.m_pfile == profile->m_name)
		{
			// Note: you need to increment it before deleting it,
			// since it becomes invalidated once erased. it++ increments
			// it but returns the original iterrator value for .erase();
			m_nodes.erase(it++);
		}
		else
		{
			++it;
		}
	}

	// If the doppelganger uses this profile, change it to default
	if (m_doppelganger.m_pfile == profile->m_name)
	{
		m_doppelganger.m_pfile = "default";
		m_doppelganger.m_portSetIndex = 0;
	}

	for (uint i = 0; i < profile->m_parent->m_children.size(); i++)
	{
		if (profile->m_parent->m_children[i] == profile)
		{
			profile->m_parent->m_children.erase(profile->m_parent->m_children.begin() + i);
			break;
		}
	}

	delete profile;

	return true;
}

bool HoneydConfiguration::AddScript(Script script)
{
	if(GetScript(script.m_name).m_osclass != "")
	{
		return false;
	}
	m_scripts.push_back(script);
	return true;
}

Script HoneydConfiguration::GetScript(string name)
{
	for(uint i = 0; i < m_scripts.size(); i++)
	{
		if(m_scripts[i].m_name == name)
		{
			return m_scripts[i];
		}
	}

	//Return empty script
	return Script();
}

bool HoneydConfiguration::AddScriptOptionValue(string scriptName, string keyName, string value)
{
	for(uint i = 0; i < m_scripts.size(); i++)
	{
		if(m_scripts[i].m_name == scriptName)
		{

			// Check for duplicates
			for (vector<string>::iterator it = m_scripts[i].options[keyName].begin(); it != m_scripts[i].options[keyName].end(); it++)
			{
				if ((*it) == value)
				{
					return false;
				}
			}

			m_scripts[i].options[keyName].push_back(value);
			return true;
		}
	}
	return false;
}

bool HoneydConfiguration::DeleteScriptOptionValue(string scriptName, string keyName, string value)
{
	for(uint i = 0; i < m_scripts.size(); i++)
	{
		if(m_scripts[i].m_name == scriptName)
		{
			for (vector<string>::iterator it = m_scripts[i].options[keyName].begin(); it != m_scripts[i].options[keyName].end(); it++)
			{
				if ((*it) == value)
				{
					m_scripts[i].options[keyName].erase(it);
					return true;
				}
			}
				break;
		}
	}
	return false;
}

vector<Script> HoneydConfiguration::GetScripts(std::string service, std::string osclass)
{
	vector<Script> ret;

	for(uint i = 0; i < m_scripts.size(); i++)
	{
		if((m_scripts[i].m_service == service) && (m_scripts[i].m_osclass == osclass))
		{
			ret.push_back(m_scripts[i]);
		}
	}

	return ret;
}

bool HoneydConfiguration::DeleteScript(string name)
{
	for(uint i = 0; i < m_scripts.size(); i++)
	{
		if(m_scripts[i].m_name == name)
		{
			m_scripts.erase(m_scripts.begin()+i);
			//Also remove any references to this script from any profiles
			DeleteScriptFromPorts(name);
			return true;
		}
	}

	return false;
}

string HoneydConfiguration::SanitizeProfileName(std::string oldName)
{
	if (!oldName.compare("default") || !oldName.compare(""))
	{
		return oldName;
	}

	string newname = "pfile" + oldName;
	ReplaceString(newname, " ", "-");
	ReplaceString(newname, ",", "COMMA");
	ReplaceString(newname, ";", "SEMICOLON");
	ReplaceString(newname, "@", "AT");
	return newname;
}

void HoneydConfiguration::ClearProfiles()
{
	for (uint i = 0; i < m_profiles.m_root->m_children.size(); i++)
	{
		delete m_profiles.m_root->m_children[i];
	}
	m_profiles.m_root->m_children.clear();
}

void HoneydConfiguration::ClearNodes()
{
	m_nodes.clear();
}


bool HoneydConfiguration::SwitchToConfiguration(const string& configName)
{
	bool found = false;

	for(uint i = 0; i < m_configs.size(); i++)
	{
		if(!m_configs[i].compare(configName))
		{
			found = true;
		}
	}

	if(found)
	{
		Config::Inst()->SetCurrentConfig(configName);
		return true;
	}
	else
	{
		cout << "No configuration with name " << configName << " found, doing nothing" << endl;
		return false;
	}
}

bool HoneydConfiguration::AddNewConfiguration(const string& configName, bool clone, const string& cloneConfig)
{
	bool found = false;

	if(configName.empty())
	{
		cout << "Empty string is not acceptable configuration name, exiting" << endl;
		return false;
	}

	for(uint i = 0; i < m_configs.size(); i++)
	{
		if(!m_configs[i].compare(configName))
		{
			cout << "Cannot add configuration with the same name as existing configuration" << endl;
			return false;
		}
		if(clone && !m_configs[i].compare(cloneConfig))
		{
			found = true;
		}
	}

	if(clone && !found)
	{
		cout << "Cannot find configuration " << cloneConfig << " to clone, exiting" << endl;
		return false;
	}

	m_configs.push_back(configName);

	ofstream addfile(Config::Inst()->GetPathHome() + "/config/templates/configurations.txt", ios_base::app);

	if(!clone)
	{
		// Add configName to configurations.txt within the templates/ folder,
		// create the templates/configName/ directory, and fill with
		// empty (but still parseable) xml files
		boost::filesystem::path directoryPath = Config::Inst()->GetPathHome() + "/config/templates/" + configName + "/";
		boost::filesystem::create_directories(directoryPath);

		string oldName = Config::Inst()->GetCurrentConfig();
		ReadAllTemplatesXML();
		Config::Inst()->SetCurrentConfig(configName);

	    ClearNodes();
		ClearProfiles();

		addfile << configName << '\n';
		addfile.close();
		WriteAllTemplatesToXML();

		Config::Inst()->SetCurrentConfig(oldName);
		return true;
	}
	else if(clone && found)
	{
		// Add configName to configurations.txt within the templates/ folder,
		// create the templates/configName/ directory, and cp the
		// stuff from templates/cloneConfig/ into it.

		// Make a function for recursively copying a directory

		boost::filesystem::path fromString = Config::Inst()->GetPathHome() + "/config/templates/" + cloneConfig + "/";
		boost::filesystem::path toString = Config::Inst()->GetPathHome() + "/config/templates/" + configName + "/";

		RecursiveDirectoryCopy(fromString, toString, true);

		addfile << configName << '\n';
		addfile.close();
		return true;
	}
	return false;
}

bool HoneydConfiguration::RemoveConfiguration(const std::string& configName)
{
	if(m_configs.size() == 1)
	{
		LOG(ERROR, "You cannot delete all haystack configurations", "");
		return false;
	}

	bool found = false;

	uint eraseIdx = 0;

	for(uint i = 0; i < m_configs.size(); i++)
	{
		if(!m_configs[i].compare(configName))
		{
			found = true;
			eraseIdx = i;
		}
	}

	if(found)
	{
		boost::filesystem::path pathToDelete = Config::Inst()->GetPathHome() + "/config/templates/" + configName + "/";
		try
		{
			boost::filesystem::remove_all(pathToDelete);
		}
		catch(boost::filesystem::filesystem_error err)
		{
			LOG(INFO, "", "The name of the configuration to remove was in the m_configs array, but the folders haven't been created. Removing name from list.");
		}
		int oldSize = 0;
		for(uint i = 0; i < m_configs.size(); i++)
		{
			if(m_configs[i].compare(""))
			{
				oldSize++;
			}
		}
		int newSize = oldSize - 1;
		m_configs.erase(m_configs.begin() + eraseIdx);
		m_configs.resize(newSize);
		ofstream configurationsFile(Config::Inst()->GetPathHome() + "/config/templates/configurations.txt");
		string writeString = "";
		for(uint i = 0; i < m_configs.size(); i++)
		{
			if(m_configs[i].compare(""))
			{
				writeString += m_configs[i] + '\n';
			}
		}
		configurationsFile << writeString;
		configurationsFile.close();
		return true;
	}
	else
	{
		cout << "No configuration with name " << configName << ", exiting" << endl;
		return false;
	}
}

bool HoneydConfiguration::LoadConfigurations()
{
	string configurationPath = Config::Inst()->GetPathHome() + "/config/templates/configurations.txt";

	ifstream configList(configurationPath);

	m_configs.clear();

	while(configList.good())
	{
		string pushback;
		getline (configList,pushback);
		m_configs.push_back(pushback);
	}

	return true;
}

vector<string> HoneydConfiguration::GetConfigurationsList()
{
	LoadConfigurations();
	return m_configs;
}

bool HoneydConfiguration::SetDoppelganger(Node doppelganger)
{
	m_doppelganger = doppelganger;
	return true;
}

Node HoneydConfiguration::GetDoppelganger()
{
	return m_doppelganger;
}

Profile* HoneydConfiguration::GetRoot()
{
	return m_profiles.m_root;
}

vector<string> HoneydConfiguration::GetLeafProfileNames()
{
	return GetLeafProfileNames_helper(m_profiles.m_root);
}

vector<string> HoneydConfiguration::GetLeafProfileNames_helper(Profile *item)
{
	if(item == NULL)
	{
		return vector<string>();
	}

	vector<string> ret;
	if(item->m_children.size() == 0)
	{
		ret.push_back(item->m_name);
		return ret;
	}

	for(uint i = 0; i < item->m_children.size(); i++)
	{
		vector<string> childVec = GetLeafProfileNames_helper(item->m_children[i]);
		ret.insert(ret.end(), childVec.begin(), childVec.end());
	}

	return ret;
}

}
