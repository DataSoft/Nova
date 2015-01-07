//============================================================================
// Name        : HoneydHostConfig.cpp
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
// Description : Main HH_CONFIG file, performs the subnet acquisition, scanning, and
//               parsing of the resultant .xml output into a PersonalityTable object
//============================================================================


// REQUIRES NMAP 6

#include <time.h>
#include <stdio.h>
#include <netdb.h>
#include <sstream>
#include <fstream>
#include <ctype.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <csignal>
#include <sys/stat.h>
#include <fcntl.h>

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "HaystackAutoConfig.h"
#include "HoneydConfiguration/HoneydConfiguration.h"
#include "HoneydConfiguration/ProfileTree.h"
#include "HoneydConfiguration/VendorMacDb.h"
#include "HoneydConfiguration/Subnet.h"
#include "Logger.h"

using namespace std;
using namespace Nova;

vector<string> interfacesToMatch;
vector<string> subnetsToAdd;
string localMachine;
string nmapFileName;
uint numNodes;
double nodeRatio;
string nodeRange;
string nodeInterface;
vector<pair<int, int> > nodeRangeVector;
string group;
ofstream lockFile;

enum NumberOfNodesType
{
	FIXED_NUMBER_OF_NODES,
	RATIO_BASED_NUMBER_OF_NODES,
	RANGE_BASED_NUMBER_OF_NODES
};

NumberOfNodesType numberOfNodesType;
vector<Subnet> subnetsDetected;
ScannedHostTable scannedHosts;
string lockFilePath;

void sig_handler(int x)
{
	LOG(WARNING, "HaystackAutoconfig closing on signal received", "");
	if(!group.empty())
	{
		HoneydConfiguration::Inst()->RemoveConfiguration(group);
	}
	lockFile.close();
	remove(lockFilePath.c_str());
	Config::Inst()->SetCurrentConfig("default");
	exit(HHC_CODE_RECV_SIG);
}

int main(int argc, char ** argv)
{
	//Seed any future random values
	srand(time(NULL));

	signal(SIGINT, sig_handler);
	signal(SIGKILL, sig_handler);
	signal(SIGTERM, sig_handler);

	namespace po = boost::program_options;
	po::options_description desc("Command line options");
	try
	{
		desc.add_options()
				("help,h", "Show command line options")
				("num-nodes,n", po::value<uint>(&numNodes), "Number of nodes to create (can't be used with -r)")
				("num-nodes-ratio,r", po::value<double>(&nodeRatio), "Ratio of haystack nodes to create vs real nodes (eg, 2 for 2x haystack nodes per real host or 0.5 for half the number of haystack nodes as real hosts)")
				("num-nodes-range,e", po::value<string>(&nodeRange), "Range of IPs which to assign nodes to; the range will be filled completely with haystack nodes.")
				("scaninterface,i", po::value<vector<string> >(), "Interface(s) to use for subnet selection.")
				("nodeinterface", po::value<string>(&nodeInterface), "Interface to put newly created nodes on.")
				("additional-subnet,a", po::value<vector<string> >(), "Additional subnets to scan. Must be subnets that will return Nmap results from the AutoConfig tool's location, and of the form XXX.XXX.XXX.XXX/##")
				("nmap-xml,f", po::value<string>(), "Nmap 6.00+ XML output file to parse instead of scanning. Selecting this option skips the subnet identification and scanning phases, thus the INTERFACE and ADDITIONAL-SUBNET options will do nothing.")
				("group,g", po::value<string>(), "Name for new haystack group created by the AutoConfig tool. Incompatible with (--append-to,-t) flag")
				("append-to,t", po::value<string>(), "Name of haystack group to append AutoConfig results to. Incompatible with (--group,-g) flag");

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		lockFilePath = Config::Inst()->GetPathHome() + "/data/hhconfig.lock";
		struct stat buf;
		if(stat(lockFilePath.c_str(), &buf) == 0)
		{
			LOG(ERROR, "There is already an instance of Haystack Autoconfig running, aborting...", "");
			return HHC_CODE_NO_MULTI_RUN;
		}
		lockFile.open(lockFilePath.c_str());

		bool i_flag_empty = true;
		bool a_flag_empty = true;
		bool f_flag_set = false;

		if(vm.count("help"))
		{
			cout << desc << endl;
			lockFile.close();
			remove(lockFilePath.c_str());
			exit(HHC_CODE_OKAY);
		}

		if(vm.count("group") && vm.count("append-to"))
		{
			cout << "ERROR: You can use either -g to define a new group or -t to append to a specific group, not both." << '\n';
			lockFile.close();
			remove(lockFilePath.c_str());
			exit(HHC_CODE_BAD_ARG_VALUE);
		}

		if((vm.count("num-nodes-ratio") && vm.count("num-nodes"))
			|| (vm.count("num-nodes") && vm.count("num-nodes-range"))
			|| (vm.count("num-nodes-ratio") && vm.count("num-nodes-range")))
		{
			cout << "ERROR: You can only use one of -r, -n and -e to specify the number of nodes." << '\n';
			lockFile.close();
			remove(lockFilePath.c_str());
			exit(HHC_CODE_BAD_ARG_VALUE);
		}

		if(vm.count("group"))
		{
			cout << "Creating new haystack group " << vm["group"].as<string>() << '\n';
			group = vm["group"].as<string>();
			HoneydConfiguration::Inst()->AddNewConfiguration(vm["group"].as<string>(), false, "");
			HoneydConfiguration::Inst()->SwitchToConfiguration(vm["group"].as<string>());
		}
		else if(!vm.count("group") && !vm.count("append-to"))
		{
			string defaultCreatedGroup = "autoconfig";
			time_t timestamp = time(NULL);
			stringstream ss;
			ss << timestamp;
			defaultCreatedGroup += ss.str();
			group = defaultCreatedGroup;
			cout << "No group dictated, using default group name " << defaultCreatedGroup << '\n';
			HoneydConfiguration::Inst()->AddNewConfiguration(defaultCreatedGroup, false, "");
			HoneydConfiguration::Inst()->SwitchToConfiguration(defaultCreatedGroup);
		}

		if(vm.count("append-to"))
		{
			string configGroup = vm["append-to"].as<string>();
			cout << "Appending Autoconfig results to haystack group " << configGroup << '\n';
			vector<string> configList = HoneydConfiguration::Inst()->GetConfigurationsList();
			bool exists = false;
			for(uint i = 0; i < configList.size(); i++)
			{
				if(configList[i] == configGroup)
				{
					exists = true;
				}
			}
			if(!exists)
			{
				LOG(ERROR, ("Desired group \"" + configGroup + "\" does not exist, aborting..."), "");
				lockFile.close();
				remove(lockFilePath.c_str());
				exit(HHC_CODE_BAD_ARG_VALUE);
			}
			HoneydConfiguration::Inst()->SwitchToConfiguration(vm["append-to"].as<string>());
			HoneydConfiguration::Inst()->ReadAllTemplatesXML();
		}

		if(vm.count("num-nodes-ratio"))
		{
			cout << "Number of nodes to create: " << nodeRatio << " * number of real hosts found" << '\n';
			numberOfNodesType = RATIO_BASED_NUMBER_OF_NODES;
		}

		if(vm.count("num-nodes-range"))
		{
			cout << "" << endl;
			numberOfNodesType = RANGE_BASED_NUMBER_OF_NODES;
		}

		if(vm.count("num-nodes"))
		{
			cout << "Number of nodes to create: " << numNodes << '\n';
			numberOfNodesType = FIXED_NUMBER_OF_NODES;
		}

		if (!vm.count("nodeinterface"))
		{
			nodeInterface = Config::Inst()->GetInterface(0);
		}

		cout << "Creating nodes on interface " << nodeInterface << endl;
		if(vm.count("scaninterface"))
		{
			vector<string> interfaces_flag = vm["scaninterface"].as< vector<string> >();

			for(uint i = 0; i < interfaces_flag.size(); i++)
			{
				if(interfaces_flag[i].find(",") != string::npos)
				{
					vector<string> splitInterfacesFlagI;
					string split = interfaces_flag[i];
					boost::split(splitInterfacesFlagI, split, boost::is_any_of(","));

					for(uint j = 0; j < splitInterfacesFlagI.size(); j++)
					{
						if(!splitInterfacesFlagI[j].empty())
						{
							interfacesToMatch.push_back(splitInterfacesFlagI[j]);
						}
					}
				}
				else
				{
					interfacesToMatch.push_back(interfaces_flag[i]);
				}
			}
			i_flag_empty = false;
		}
		if(vm.count("additional-subnet"))
		{
			vector<string> addsubnets_flag = vm["additional-subnet"].as< vector<string> >();

			for(uint i = 0; i < addsubnets_flag.size(); i++)
			{
				if(addsubnets_flag[i].find(",") != string::npos)
				{
					vector<string> splitSubnetsFlagI;
					string split = addsubnets_flag[i];
					boost::split(splitSubnetsFlagI, split, boost::is_any_of(","));

					for(uint j = 0; j < splitSubnetsFlagI.size(); j++)
					{
						if(!splitSubnetsFlagI[j].empty())
						{
							subnetsToAdd.push_back(splitSubnetsFlagI[j]);
						}
					}
				}
				else
				{
					subnetsToAdd.push_back(addsubnets_flag[i]);
				}
			}

			a_flag_empty = false;
		}
		if(vm.count("nmap-xml"))
		{
			cout << "file to parse is " << vm["nmap-xml"].as< string >() << '\n';
			nmapFileName = vm["nmap-xml"].as<string>();
			f_flag_set = true;
		}

		if((numberOfNodesType == FIXED_NUMBER_OF_NODES) && (numNodes < 0))
		{
			HoneydConfiguration::Inst()->RemoveConfiguration(group);
			lockFile.close();
			remove(lockFilePath.c_str());
			LOG(ERROR, "num-nodes argument takes an integer greater than or equal to 0. Aborting...", "");
			exit(HHC_CODE_BAD_ARG_VALUE);
		}

		if(numberOfNodesType == RATIO_BASED_NUMBER_OF_NODES && nodeRatio < 0)
		{
			HoneydConfiguration::Inst()->RemoveConfiguration(group);
			lockFile.close();
			remove(lockFilePath.c_str());
			LOG(ERROR, "num-nodes ratio argument must be greater than or equal to 0. Aborting...", "");
			exit(HHC_CODE_BAD_ARG_VALUE);
		}

		if(numberOfNodesType == RANGE_BASED_NUMBER_OF_NODES && (nodeRange.empty() || false /* this will be a regular expression format */))
		{
			HoneydConfiguration::Inst()->RemoveConfiguration(group);
			lockFile.close();
			remove(lockFilePath.c_str());
			LOG(ERROR, "num-nodes range argument was given no value or is in an incorrect format. Aborting...", "");
			exit(HHC_CODE_BAD_ARG_VALUE);
		}

		HHC_ERR_CODE errVar = HHC_CODE_OKAY;

		// Arg parsing done, moving onto execution items
		if(f_flag_set)
		{
			LOG(ALERT, "Launching Haystack Auto-configuration Tool", "");

			if(!HoneydConfiguration::Inst()->ReadScriptsXML())
			{
				LOG(ERROR, "Problem reading script template XML from file", "");
				lockFile.close();
				remove(lockFilePath.c_str());
				exit(HHC_CODE_PARSING_ERROR);
			}

			if(!LoadNmapXML(nmapFileName))
			{
				HoneydConfiguration::Inst()->RemoveConfiguration(group);
				LOG(ERROR, "LoadNmapXML failed. Aborting...", "");
				lockFile.close();
				remove(lockFilePath.c_str());
				exit(HHC_CODE_PARSING_ERROR);
			}

			GenerateConfiguration();
			lockFile.close();
			remove(lockFilePath.c_str());
			LOG(INFO, "Honeyd profile and node configuration completed.", "");
			return errVar;
		}
		else if(a_flag_empty && i_flag_empty)
		{
			HoneydConfiguration::Inst()->RemoveConfiguration(group);
			errVar = HHC_CODE_REQUIRED_FLAGS_MISSING;
			lockFile.close();
			remove(lockFilePath.c_str());
			LOG(ERROR, "Must designate an Nmap XML file to parse, or provide either an interface or a subnet to scan. Aborting...", "");
			cout << endl << desc << endl;
			return errVar;
		}
		else
		{
			LOG(ALERT, "Launching Honeyd Host Configuration Tool", "");
			vector<string> subnetNames;

			if(!HoneydConfiguration::Inst()->ReadScriptsXML())
			{
				LOG(ERROR, "Problem reading script template XML from file", "");
				lockFile.close();
				remove(lockFilePath.c_str());
				exit(HHC_CODE_PARSING_ERROR);
			}

			if(!i_flag_empty)
			{
				subnetNames = GetSubnetsToScan(&errVar, interfacesToMatch);
			}
			if(!a_flag_empty)
			{
				for(uint i = 0; i < subnetsToAdd.size(); i++)
				{
					subnetNames.push_back(subnetsToAdd[i]);
				}
			}

			cout << "Scanning following subnets: " << endl;
			for(uint i = 0; i < subnetNames.size(); i++)
			{
				cout << "\t" << subnetNames[i] << endl;
			}
			cout << endl;

			errVar = LoadPersonalityTable(subnetNames);

			if(errVar != HHC_CODE_OKAY)
			{
				HoneydConfiguration::Inst()->RemoveConfiguration(group);
				lockFile.close();
				remove(lockFilePath.c_str());
				LOG(ERROR, "There was a problem loading the PersonalityTable. Aborting...", "");
				return errVar;
			}

			GenerateConfiguration();
			lockFile.close();
			remove(lockFilePath.c_str());
			LOG(INFO, "Honeyd profile and node configuration completed.", "");
			return errVar;
		}
	}
	catch(exception &e)
	{
		LOG(ERROR, "Uncaught exception: " + string(e.what()) + ".", "");
		lockFile.close();
		remove(lockFilePath.c_str());
		cout << '\n' << desc << endl;
		return HHC_CODE_GENERIC_ERROR;
	}
}

void Nova::ParseHost(boost::property_tree::ptree propTree)
{
	using boost::property_tree::ptree;

	// Instantiate Personality object here, populate it from the code below
	ScannedHost *newHost = new ScannedHost();

	VendorMacDb *macVendorDB = new VendorMacDb();
	macVendorDB->LoadPrefixFile();

	// For the personality table, increment the number of hosts found
	// and decrement the number of hosts available, so at the end
	// of the configuration process we don't over-allocate space
	// in the hostspace for the subnets, clobbering existing DHCP
	// allocations.
	scannedHosts.m_num_of_hosts++;

	//Parse the OS first, as we need its value for other calculations
	BOOST_FOREACH(ptree::value_type &value, propTree.get_child("os"))
	{
		if(!value.first.compare("osmatch") && newHost->m_personalityClass.empty())
		{
			try
			{
				if(value.second.get<string>("<xmlattr>.name").compare(""))
				{
					newHost->m_personality = value.second.get<string>("<xmlattr>.name");
					newHost->m_personalityClass.push_back(value.second.get<string>("<xmlattr>.name"));
				}
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
			{
				LOG(DEBUG, "Caught Exception: " + string(e.what()), "");
			}
			try
			{
				if(value.second.get<string>("osclass.<xmlattr>.osgen").compare(""))
				{
					newHost->m_personalityClass.push_back(value.second.get<string>("osclass.<xmlattr>.osgen"));
				}
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
			{
				LOG(DEBUG, "Caught Exception: " + string(e.what()), "");
			}
			try
			{
				if(value.second.get<string>("osclass.<xmlattr>.osfamily").compare(""))
				{
					newHost->m_personalityClass.push_back(value.second.get<string>("osclass.<xmlattr>.osfamily"));
				}
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
			{
				LOG(DEBUG, "Caught Exception: " + string(e.what()), "");
			}
			try
			{
				if(value.second.get<string>("osclass.<xmlattr>.vendor").compare(""))
				{
					newHost->m_personalityClass.push_back(value.second.get<string>("osclass.<xmlattr>.vendor"));
				}
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
			{
				LOG(DEBUG, "Caught Exception: " + string(e.what()), "");
			}
		}
	}

	//Parse the rest of the xml tags
	BOOST_FOREACH(ptree::value_type &value, propTree.get_child(""))
	{
		try
		{
			// If we've found the <address> tags within the <host> xml node
			if(!value.first.compare("address"))
			{
				// and then find the ipv4 address, add it to the addresses
				// vector for the personality object;
				if(!value.second.get<string>("<xmlattr>.addrtype").compare("ipv4"))
				{
					// If we're not parsing ourself, just get the address in the
					// <addr> tag
					if(localMachine.compare(value.second.get<string>("<xmlattr>.addr")))
					{
						newHost->m_addresses.push_back(value.second.get<string>("<xmlattr>.addr"));
					}
					// If, however, we're parsing ourself, we need to do some extra work.
					// The IP address will be in the nmap XML structure, but the MAC will not.
					// Thus, we need to get it through other means. This will grab a string
					// representation of the MAC address, convert the first three hex pairs into
					// an unsigned integer, and then find the MAC vendor using the VendorMacDb class
					// of Nova proper.
					else
					{
						newHost->m_addresses.push_back(value.second.get<string>("<xmlattr>.addr"));

						string vendorString = "";
						string macString = "";

						struct ifreq buffer;
						vector<string> interfacesFromConfig = Config::Inst()->ListInterfaces();

						for(uint j = 0; j < interfacesFromConfig.size() && macString.empty(); j++)
						{
							int socketFD = socket(PF_INET, SOCK_DGRAM, 0);

							memset(&buffer, 0x00, sizeof(buffer));
							strncpy(buffer.ifr_name, interfacesFromConfig[j].c_str(), sizeof(buffer.ifr_name));
							ioctl(socketFD, SIOCGIFHWADDR, &buffer);
							close(socketFD);

							char macPair[3];

							stringstream ss;
							string zeroCheck;

							for(uint k = 0; k < 6; k++)
							{
								sprintf(macPair, "%2x", (unsigned char)buffer.ifr_hwaddr.sa_data[k]);
								zeroCheck = string(macPair);

								zeroCheck = boost::trim_left_copy(zeroCheck);
								zeroCheck = boost::trim_right_copy(zeroCheck);

								if(!zeroCheck.compare("0"))
								{
									ss << zeroCheck << "0";
								}
								else
								{
									ss << macPair;
								}
								if(k != 5)
								{
									ss << ":";
								}
							}

							macString = ss.str();
							ss.str("");
						}

						newHost->m_macs.push_back(macString);

						uint rawMACPrefix = macVendorDB->AtoMACPrefix(macString);
						vendorString = macVendorDB->LookupVendor(rawMACPrefix);

						if(!vendorString.empty())
						{
							newHost->AddVendor(vendorString);
						}
					}
				}
				// if we've found the MAC, add the hardware address to the MACs
				// vector in the Personality object and then add the vendor to
				// the MAC_Table inside the object as well.
				else if(!value.second.get<string>("<xmlattr>.addrtype").compare("mac"))
				{
					newHost->m_macs.push_back(value.second.get<string>("<xmlattr>.addr"));
					newHost->AddVendor(value.second.get<string>("<xmlattr>.vendor"));
				}
			}
			// If we've found the <ports> tag within the <host> xml node
			else if(!value.first.compare("ports"))
			{
				PortSet *newPortSet = new PortSet();
				newHost->m_portSets.push_back(newPortSet);

				BOOST_FOREACH(ptree::value_type &portValue, value.second.get_child(""))
				{
					try
					{
						// try, for every tag in <ports>, to get the
						// port number, the protocol (tcp, udp, sctp, etc...)
						// and the name of the <service> running on the port

						if(!portValue.first.compare("extraports"))
						{
							//XXX: Assume TCP for now, because there is no tag for protocol from Nmap here
							string defaultBehavior = portValue.second.get<string>("<xmlattr>.state");
							if(!defaultBehavior.empty())
							{
								if(!newPortSet->SetTCPBehavior(defaultBehavior))
								{
									LOG(WARNING, string("SetTCPBehavoir with defaultBehavior \"") + defaultBehavior + string("\" for newPortSet failed"), "");
								}
								continue;
							}
						}

						stringstream ss;
						uint portNumber = portValue.second.get<uint>("<xmlattr>.portid");


						string protocolString = portValue.second.get<string>("<xmlattr>.protocol");
						enum PortProtocol protocol = Port::StringToPortProtocol(protocolString);
						if(protocol == PROTOCOL_ERROR)
						{
							continue;
						}

						string serviceName = portValue.second.get<string>("service.<xmlattr>.name");

						string portState = portValue.second.get<string>("state.<xmlattr>.state");
						enum PortBehavior behavior = Port::StringToPortBehavior(portState);
						if(behavior == PORT_ERROR)
						{
							//ERROR
							continue;
						}

						//Add this port into the running port set
						Port port(serviceName, protocol, portNumber, behavior);

						if(behavior == PORT_OPEN)
						{
							//We need all the OS strings to have been read
							if(newHost->m_personalityClass.size() > 3)
							{
								//Form the correct OS string to do the script lookup: "vendor | family"
								string osclass = newHost->m_personalityClass[3] + " | " + newHost->m_personalityClass[2];

								//Do a lookup to see if there are scripts for this open port
								vector<Script> validScripts = HoneydConfiguration::Inst()->GetScripts(
										serviceName, osclass);
								if(!validScripts.empty())
								{
									//Pick one of the scripts at random
									uint random = rand() % validScripts.size();

									port.m_scriptName = validScripts[random].m_name;
									port.m_behavior = PORT_SCRIPT;
								}
							}
						}

						newPortSet->AddPort(port);
					}
					catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
					{
						LOG(DEBUG, "Caught Exception : " + string(e.what()), "");
					}
				}
			}
			else if(!value.first.compare("uptime"))
			{
				try
				{
					newHost->m_uptime = value.second.get<uint>("<xmlattr>.seconds");
				}
				catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
				{
					LOG(DEBUG, "Error parsing nmap XML uptime attribute: " + string(e.what()), "");
				}
			}
		}
		catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
		{
			LOG(DEBUG, "Caught Exception : " + string(e.what()), "");
		}
	}

	// If personalityClass vector is empty, assign this profile to the NULL fake profile
	if(newHost->m_personalityClass.empty())
	{
		newHost->m_personalityClass.push_back(Profile::EMPTY_PROFILE_NAME);
		newHost->m_osclass = Profile::EMPTY_PROFILE_NAME;
	}
	else
	{
		// Generate OS Class strings for use later down the line; used primarily
		// for matching open ports to scripts in the script table. So, say 22_TCP is open
		// on a host, we'll use the m_personalityClass string to match the OS and open port
		// to a script and then assign that script automatically.
		for(uint i = 0; i < newHost->m_personalityClass.size(); i++)
		{
			newHost->m_osclass += newHost->m_personalityClass[i];
			newHost->m_osclass += " | ";
		}
	}

	// Call AddHost() on the Personality object created at the beginning of this method
	scannedHosts.AddHost(newHost);
}

bool Nova::LoadNmapXML(const string &filename)
{
	using boost::property_tree::ptree;
	ptree propTree;

	if(filename.empty())
	{
		LOG(ERROR, "Empty string passed to LoadNmapXml", "");
		return false;
	}
	// Read the nmap xml output file (given by the filename parameter) into a boost
	// property tree for parsing of the found hosts.
	try
	{
		read_xml(filename, propTree);

		BOOST_FOREACH(ptree::value_type &host, propTree.get_child("nmaprun"))
		{
			if(!host.first.compare("host"))
			{
				ptree tempPropTree = host.second;

				// Call ParseHost on the <host> subtree within the xml file.
				ParseHost(tempPropTree);
			}
		}
	}
	catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
	{
		LOG(ERROR, "Couldn't parse XML file (bad path): " + string(e.what()) + ".", "");
		return false;
	}
	catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::xml_parser::xml_parser_error> > &e)
	{
		LOG(ERROR, "Couldn't parse from file (parser error) " + filename + ": " + string(e.what()) + ".", "");
		return false;
	}
	catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_data> > &e)
	{
		LOG(DEBUG, "Couldn't parse XML file (bad data): " + string(e.what()) + ".", "");
		return false;
	}
	return true;
}

Nova::HHC_ERR_CODE Nova::LoadPersonalityTable(vector<string> subnetNames)
{
	if(subnetNames.empty())
	{
		LOG(ERROR, "Passed empty vector of subnet names to LoadPersonalityTable.", "");
		return HHC_CODE_BAD_FUNCTION_PARAM;
	}

	stringstream ss;
	// For each element in recv (which contains strings of the subnets),
	// do an OS fingerprinting scan and output the results into a different
	// xml file for each subnet.
	for(uint16_t i = 0; i < subnetNames.size(); i++)
	{
		ss << i;
		string executionString = "sudo nmap -T4 -O --osscan-guess --stats-every 1s -oX  " + Config::Inst()->GetPathHome() + "/data/subnet" + ss.str() + ".xml " + subnetNames[i];
		LOG(INFO, "Running scan: " + executionString, "");

		//popen here for stdout of nmap
		for(uint j = 0; j < 3; j++)
		{
			stringstream s2;
			s2 << (j + 1);
			LOG(INFO, "Attempt " + s2.str() + " to start Nmap scan.", "");
			s2.str("");

			FILE* nmap = popen(executionString.c_str(), "r");

			if(nmap == NULL)
			{
				LOG(ERROR, "Couldn't start Nmap.", "");
				return HHC_CODE_NO_NMAP;
			}
			else
			{
				char buffer[256];
				while(!feof(nmap))
				{
					if(fgets(buffer, 256, nmap) != NULL)
					{
						// Would avoid using endl, but need it for what this line is being used for (printing nmap output to Web UI)
						cout << string(buffer) << endl;
					}
				}
			}

			pclose(nmap);
			j = 3;
		}

		try
		{
			string file = Config::Inst()->GetPathHome() + "/data/subnet" + ss.str() + ".xml";
			// Call LoadNmap on the generated filename so that we
			// can add hosts to the personality table, provided they
			// contain enough data.
			if(!LoadNmapXML(file))
			{
				return HHC_CODE_PARSING_ERROR;
			}
		}
		catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
		{
			LOG(DEBUG, string("Caught Exception : ") + e.what(), "");
			return HHC_CODE_PARSING_ERROR;
		}
		catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_data> > &e)
		{
			LOG(DEBUG, string("Caught Exception : ") + e.what(), "");
			return HHC_CODE_PARSING_ERROR;
		}
		ss.str("");
	}
	return HHC_CODE_OKAY;
}

void Nova::PrintStringVector(vector<string> stringVector)
{
	// Debug method to output what subnets were found by
	// the GetSubnetsToScan() method.
	stringstream ss;
	ss << "Subnets to be scanned: ";
	for(uint16_t i = 0; i < stringVector.size(); i++)
	{
		ss <<  stringVector[i] << " & ";
	}
	LOG(DEBUG, ss.str(), "");
}

vector<string> Nova::GetSubnetsToScan(Nova::HHC_ERR_CODE *errVar, vector<string> interfacesToMatch)
{
	vector<string> hostAddrStrings;
	hostAddrStrings.clear();

	if(errVar == NULL || interfacesToMatch.empty())
	{
		LOG(ERROR, "errVar is NULL or empty vector of interfaces passed to GetSubnetsToScan", "");
		return hostAddrStrings;
	}

	struct ifaddrs *devices = NULL;
	ifaddrs *curIf = NULL;
	stringstream ss;

	char addrBuffer[NI_MAXHOST];
	char bitmaskBuffer[NI_MAXHOST];
	struct in_addr netOrderAddrStruct;
	struct in_addr netOrderBitmaskStruct;
	struct in_addr minAddrInRange;
	struct in_addr maxAddrInRange;
	uint32_t hostOrderAddr;
	uint32_t hostOrderBitmask;

	// If we call getifaddrs and it fails to obtain any information, no point in proceeding.
	// Return the empty addresses vector and set the ErrorCode to AUTODETECTFAIL.
	if(getifaddrs(&devices))
	{
		LOG(ERROR, "Ethernet Interface Auto-Detection failed" , "");
		*errVar = HHC_CODE_AUTODETECT_FAIL;
		return hostAddrStrings;
	}

	vector<string> interfaces;

	// For every found interface, we need to do some processing.
	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		// IF we've found a loopback address with an IPv4 address
		if((curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_INET))
		{
			Subnet newSubnet;
			// start processing it to generate the subnet for the interface.
			interfaces.push_back(string(curIf->ifa_name));

			// Get the string representation of the interface's IP address,
			// and put it into the host character array.
			int socket = getnameinfo(curIf->ifa_addr, sizeof(sockaddr_in), addrBuffer, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(socket != 0)
			{
				// If getnameinfo returned an error, stop processing for the
				// method, assign the proper errorCode, and return an empty
				// vector.
				LOG(WARNING, "Getting Name info of Interface IP failed", "");
				*errVar = HHC_CODE_GET_NAMEINFO_FAIL;
				return hostAddrStrings;
			}

			// Do the same thing as the above, but for the netmask of the interface
			// as opposed to the IP address.
			socket = getnameinfo(curIf->ifa_netmask, sizeof(sockaddr_in), bitmaskBuffer, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(socket != 0)
			{
				// If getnameinfo returned an error, stop processing for the
				// method, assign the proper errorCode, and return an empty
				// vector.
				LOG(WARNING, "Getting Name info of Interface Netmask failed", "");
				*errVar = HHC_CODE_GET_BITMASK_FAIL;
				return hostAddrStrings;
			}
			// Convert the bitmask and host address character arrays to strings
			// for use later
			string bitmaskString = string(bitmaskBuffer);
			string addrString = string(addrBuffer);

			// Put the network ordered address values into the
			// address and bitmaks in_addr structs, and then
			// convert them to host longs for use in
			// determining how much hostspace is empty
			// on this interface's subnet.
			inet_aton(addrString.c_str(), &netOrderAddrStruct);
			inet_aton(bitmaskString.c_str(), &netOrderBitmaskStruct);
			hostOrderAddr = ntohl(netOrderAddrStruct.s_addr);
			hostOrderBitmask = ntohl(netOrderBitmaskStruct.s_addr);

			// Get the base address for the subnet
			uint32_t hostOrderMinAddrInRange = hostOrderBitmask & hostOrderAddr;
			minAddrInRange.s_addr = htonl(hostOrderMinAddrInRange);

			// and the max address
			uint32_t hostOrderMaxAddrInRange = ~(hostOrderBitmask) + hostOrderMinAddrInRange;
			maxAddrInRange.s_addr = htonl(hostOrderMaxAddrInRange);

			// Find out how many bits there are to work with in
			// the subnet (i.e. X.X.X.X/24? X.X.X.X/31?).
			uint32_t tempRawMask = ~hostOrderBitmask;
			int i = 32;

			while(tempRawMask != 0)
			{
				tempRawMask /= 2;
				i--;
			}

			ss << i;
			// Generate a string of the form X.X.X.X/## for use in nmap scans later
			addrString = string(inet_ntoa(minAddrInRange)) + "/" + ss.str();
			ss.str("");

			// Populate the subnet struct for use in the SubnetTable of the HoneydConfiguration
			// object.
			newSubnet.m_address = addrString;
			newSubnet.m_mask = string(inet_ntoa(netOrderBitmaskStruct));
			newSubnet.m_maskBits = i;
			newSubnet.m_base = minAddrInRange.s_addr;
			newSubnet.m_max = maxAddrInRange.s_addr;
			newSubnet.m_name = string(curIf->ifa_name);
			newSubnet.m_enabled = (curIf->ifa_flags & IFF_UP);
			newSubnet.m_isRealDevice = true;

			// Want to add loopbacks to the subnets (for Doppelganger) but not to the
			// addresses to scan vector
			if(!CheckSubnet(hostAddrStrings, addrString))
			{
				subnetsDetected.push_back(newSubnet);
			}
		}
		// If we've found an interface that has an IPv4 address and isn't a loopback
		if(!(curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_INET))
		{
			bool go = false;

			for(uint i = 0; i < interfacesToMatch.size(); i++)
			{
				if(!string(curIf->ifa_name).compare(interfacesToMatch[i]))
				{
					go = true;
				}
			}

			if(!go)
			{
				continue;
			}

			Subnet newSubnet;
			// start processing it to generate the subnet for the interface.
			interfaces.push_back(string(curIf->ifa_name));

			// Get the string representation of the interface's IP address,
			// and put it into the host character array.
			int s = getnameinfo(curIf->ifa_addr, sizeof(sockaddr_in), addrBuffer, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(s != 0)
			{
				// If getnameinfo returned an error, stop processing for the
				// method, assign the proper errorCode, and return an empty
				// vector.
				LOG(WARNING, "Getting Name info of Interface IP failed", "");
				*errVar = HHC_CODE_GET_NAMEINFO_FAIL;
				return hostAddrStrings;
			}

			// Do the same thing as the above, but for the netmask of the interface
			// as opposed to the IP address.
			s = getnameinfo(curIf->ifa_netmask, sizeof(sockaddr_in), bitmaskBuffer, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(s != 0)
			{
				// If getnameinfo returned an error, stop processing for the
				// method, assign the proper errorCode, and return an empty
				// vector.
				LOG(WARNING, "Getting Name info of Interface Netmask failed", "");
				*errVar = HHC_CODE_GET_BITMASK_FAIL;
				return hostAddrStrings;
			}
			// Convert the bitmask and host address character arrays to strings
			// for use later
			string bitmaskString = string(bitmaskBuffer);
			string addrString = string(addrBuffer);
			if(localMachine.empty())
			{
				localMachine = addrString;
			}

			// Put the network ordered address values into the
			// address and bitmaks in_addr structs, and then
			// convert them to host longs for use in
			// determining how much hostspace is empty
			// on this interface's subnet.
			inet_aton(addrString.c_str(), &netOrderAddrStruct);
			inet_aton(bitmaskString.c_str(), &netOrderBitmaskStruct);
			hostOrderAddr = ntohl(netOrderAddrStruct.s_addr);
			hostOrderBitmask = ntohl(netOrderBitmaskStruct.s_addr);

			// Get the base address for the subnet
			uint32_t hostOrderMinAddrInRange = hostOrderBitmask & hostOrderAddr;
			minAddrInRange.s_addr = htonl(hostOrderMinAddrInRange);

			// and the max address
			uint32_t hostOrderMaxAddrInRange = ~(hostOrderBitmask) + hostOrderMinAddrInRange;
			maxAddrInRange.s_addr = htonl(hostOrderMaxAddrInRange);

			// Find out how many bits there are to work with in
			// the subnet (i.e. X.X.X.X/24? X.X.X.X/31?).
			uint32_t mask = ~hostOrderBitmask;
			int i = 32;
			while(mask != 0)
			{
				mask /= 2;
				i--;
			}
			// Generate a string of the form X.X.X.X/## for use in nmap scans later
			ss << i;
			string push = string(inet_ntoa(minAddrInRange)) + "/" + ss.str();
			ss.str("");

			newSubnet.m_address = push;
			newSubnet.m_mask = string(inet_ntoa(netOrderBitmaskStruct));
			newSubnet.m_maskBits = i;
			newSubnet.m_base = minAddrInRange.s_addr;
			newSubnet.m_max = maxAddrInRange.s_addr;
			newSubnet.m_name = string(curIf->ifa_name);
			newSubnet.m_enabled = (curIf->ifa_flags & IFF_UP);
			newSubnet.m_isRealDevice = true;

			// If the subnet isn't in the addresses vector, push it in.
			if(!CheckSubnet(hostAddrStrings, push))
			{
				hostAddrStrings.push_back(push);
				subnetsDetected.push_back(newSubnet);
			}
		}
	}

	// Deallocate the devices struct from the beginning of the method,
	// and return the vector containing the subnets from each interface.
	freeifaddrs(devices);
	return hostAddrStrings;
}

void Nova::GenerateConfiguration()
{
	HoneydConfiguration::Inst()->m_profiles.LoadTable(&scannedHosts);

	int nodesToCreate = 0;
	if(numberOfNodesType == FIXED_NUMBER_OF_NODES)
	{
		nodesToCreate = numNodes;
	}
	else if(numberOfNodesType == RATIO_BASED_NUMBER_OF_NODES)
	{
		nodesToCreate = ((double)scannedHosts.m_num_of_hosts) * nodeRatio;
	}
	else
	{
		nodesToCreate = GetNumberOfIPsInRange(nodeRange);
		if(nodesToCreate == -1)
		{
			HoneydConfiguration::Inst()->RemoveConfiguration(group);
			LOG(ERROR, "There was a problem with the construction of the IP range.", "");
			return;
		}
	}

	stringstream ss;
	ss << "Creating " << nodesToCreate << " new nodes";
	LOG(DEBUG, ss.str(), "");
	if(numberOfNodesType == FIXED_NUMBER_OF_NODES || numberOfNodesType == RATIO_BASED_NUMBER_OF_NODES)
	{
		for(int i = 0; i < nodesToCreate; i++)
		{
			//Pick a (leaf) profile at random
			Profile *winningPersonality = HoneydConfiguration::Inst()->m_profiles.GetRandomProfile();
			if(winningPersonality == NULL)
			{
				continue;
			}

			//Pick a random MAC vendor from this profile
			string vendor = winningPersonality->GetRandomVendor();

			//Pick a MAC address for the node:
			string macAddress = HoneydConfiguration::Inst()->GenerateRandomUnusedMAC(vendor);



			//Make a node for that profile
			HoneydConfiguration::Inst()->AddNode(winningPersonality->m_name, "DHCP", macAddress, nodeInterface,
				winningPersonality->GetRandomPortSet());
		}
	}
	else if(numberOfNodesType == RANGE_BASED_NUMBER_OF_NODES)
	{
		for(uint k = 0; k < nodeRangeVector.size(); k++)
		{
			do
			{
				//Pick a (leaf) profile at random
				Profile *winningPersonality = HoneydConfiguration::Inst()->m_profiles.GetRandomProfile();
				if(winningPersonality == NULL)
				{
					continue;
				}

				//Pick a random MAC vendor from this profile
				string vendor = winningPersonality->GetRandomVendor();

				//Pick a MAC address for the node:
				string macAddress = HoneydConfiguration::Inst()->GenerateRandomUnusedMAC(vendor);

				//Make a node for that profile
				struct sockaddr_in start;
				start.sin_addr.s_addr = nodeRangeVector[k].first;
				char startResult[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &(start.sin_addr), startResult, INET_ADDRSTRLEN);
				string startResultString(GetReverseIp(startResult));
				HoneydConfiguration::Inst()->AddNode(winningPersonality->m_name, startResultString, macAddress, Config::Inst()->GetInterface(0),
					winningPersonality->GetRandomPortSet());
			}while((nodeRangeVector[k].first < nodeRangeVector[k].second) && nodeRangeVector[k].first++);
		}
	}

	if(!HoneydConfiguration::Inst()->WriteNodesToXML())
	{
		LOG(ERROR, "Unable to save haystack templates", "");
	}
	if(!HoneydConfiguration::Inst()->WriteProfilesToXML())
	{
		LOG(ERROR, "Unable to save haystack templates", "");
	}
	if(!HoneydConfiguration::Inst()->WriteHoneydConfiguration())
	{
		LOG(ERROR, "Unable to write haystack configuration", "");
	}
}

bool Nova::CheckSubnet(vector<string> &hostAddrStrings, string matchStr)
{
	if(!hostAddrStrings.empty() || matchStr.empty())
	{
		LOG(ERROR, "Empty vector of host address strings or empty string to match passed to CheckSubnet.", "");
		return false;
	}

	for(uint16_t j = 0; j < hostAddrStrings.size(); j++)
	{
		if(!matchStr.compare(hostAddrStrings[j]))
		{
			return true;
		}
	}
	return false;
}

int Nova::GetNumberOfIPsInRange(string ipRange)
{
	
	uint conditional = ipRange.find(',');
	if(conditional == ipRange.npos)
	{
		int split = ipRange.find('-');
		string nodeRangeStart = ipRange.substr(0, split);
		string nodeRangeEnd = ipRange.substr(split + 1, ipRange.length());

		vector<string> valueCheckStart;
		vector<string> valueCheckEnd;
		boost::split(valueCheckStart, nodeRangeStart, boost::is_any_of("."));
		boost::split(valueCheckEnd, nodeRangeEnd, boost::is_any_of("."));

		if(valueCheckEnd.size() != valueCheckStart.size())
		{
			LOG(ERROR, "Split ip vectors are of different length, aborting...", "");
			return -1;
		}

		for(uint i = 0; i < valueCheckStart.size(); i++)
		{
			int startValueI = atoi(valueCheckStart[i].c_str());
			int endValueI = atoi(valueCheckEnd[i].c_str());
			if((startValueI > 255 || startValueI < 0) || (endValueI > 255 || endValueI < 0))
			{
				LOG(ERROR, "Value within IP address out of range within user-defined IP range, aborting...", "");
				return -1;
			}
		}

		struct sockaddr_in start;
		struct sockaddr_in end;
		string inetPtonSrcStart = GetReverseIp(nodeRangeStart);
		string inetPtonSrcEnd = GetReverseIp(nodeRangeEnd);
		int retCodeStart = inet_pton(AF_INET, inetPtonSrcStart.c_str(), &(start.sin_addr));
		int retCodeEnd = inet_pton(AF_INET, inetPtonSrcEnd.c_str(), &(end.sin_addr));

		if(retCodeStart < 1 || retCodeEnd < 1)
		{
			LOG(ERROR, "inet_pton returned an error, aborting...", "");
			return -1;
		}

		pair<int, int> push;
		push.first = start.sin_addr.s_addr;
		push.second = end.sin_addr.s_addr;
		nodeRangeVector.push_back(push);

		if(start.sin_addr.s_addr > end.sin_addr.s_addr)
		{
			LOG(ERROR, "User-supplied IP range is invalid: range goes from high to low addresses", "");
			return -1;
		}
	}
	else
	{
		vector<string> ranges;
		boost::split(ranges, ipRange, boost::is_any_of(","));
		for(uint i = 0; i < ranges.size(); i++)
		{
			if(!ranges[i].empty())
			{
				int split = ranges[i].find('-');
				string nodeRangeStart = ranges[i].substr(0, split);
				string nodeRangeEnd = ranges[i].substr(split + 1, ranges[i].length());

				vector<string> valueCheckStart;
				vector<string> valueCheckEnd;
				boost::split(valueCheckStart, nodeRangeStart, boost::is_any_of("."));
				boost::split(valueCheckEnd, nodeRangeEnd, boost::is_any_of("."));

				if(valueCheckEnd.size() != valueCheckStart.size())
				{
					LOG(ERROR, "Split ip vectors are of different length, aborting...", "");
					return -1;
				}

				for(uint i = 0; i < valueCheckStart.size(); i++)
				{
					int startValueI = atoi(valueCheckStart[i].c_str());
					int endValueI = atoi(valueCheckEnd[i].c_str());
					if((startValueI > 255 || startValueI < 0) || (endValueI > 255 || endValueI < 0))
					{
						LOG(ERROR, "Value within IP address out of range within user-defined IP range, aborting...", "");
						return -1;
					}
				}

				struct sockaddr_in start;
				struct sockaddr_in end;
				string inetPtonSrcStart = GetReverseIp(nodeRangeStart);
				string inetPtonSrcEnd = GetReverseIp(nodeRangeEnd);
				int retCodeStart = inet_pton(AF_INET, inetPtonSrcStart.c_str(), &(start.sin_addr));
				int retCodeEnd = inet_pton(AF_INET, inetPtonSrcEnd.c_str(), &(end.sin_addr));

				if(retCodeStart < 1 || retCodeEnd < 1)
				{
					LOG(ERROR, "inet_pton returned an error, aborting...", "");
					return -1;
				}

				pair<int, int> push;
				push.first = start.sin_addr.s_addr;
				push.second = end.sin_addr.s_addr;
				nodeRangeVector.push_back(push);

				if(start.sin_addr.s_addr > end.sin_addr.s_addr)
				{
					LOG(ERROR, "User-supplied IP range is invalid: range goes from high to low addresses", "");
					return -1;
				}
			}
		}
	}
	return 0;
}

string Nova::GetReverseIp(string ip)
{
	string ret = "";
	vector<string> split;
	boost::split(split, ip, boost::is_any_of("."));
	reverse(split.begin(), split.end());
	for(uint i = 0; i < split.size() - 1; i++)
	{
		ret += split[i] + ".";
	}
	ret += split[split.size() - 1];
	return ret;
}
