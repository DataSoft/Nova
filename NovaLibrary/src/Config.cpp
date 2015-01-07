//============================================================================
// Name        : NOVAConfiguration.h
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
// Description : Class to load and parse the NOVA configuration file
//============================================================================

#define BOOST_FILESYSTEM_VERSION 2

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/un.h>
#include <syslog.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <pwd.h>
#include <string>
#include <iostream>

#include "Config.h"
#include "Logger.h"
#include "NovaUtil.h"
#include "Lock.h"


using namespace std;

namespace Nova
{

string Config::m_pathsFile = "/usr/share/nova/sharedFiles/paths";
string Config::m_pathPrefix = "";

string Config::m_prefixes[] =
{
	"INTERFACE",
	"KNN_NORMALIZATION",
	"HS_HONEYD_CONFIG",
	"READ_PCAP",
	"PCAP_FILE",
	"CLASSIFICATION_TIMEOUT",
	"K",
	"EPS",
	"CLASSIFICATION_THRESHOLD",
	"DOPPELGANGER_IP",
	"DOPPELGANGER_INTERFACE",
	"DM_ENABLED",
	"THINNING_DISTANCE",
	"RSYSLOG_USE",
	"RSYSLOG_IP",
	"RSYSLOG_PORT",
	"RSYSLOG_CONNTYPE",
	"SMTP_ADDR",
	"SMTP_PORT",
	"SMTP_DOMAIN",
	"SMTP_INTERVAL",
	"SMTP_PASS",
	"SMTP_USEAUTH",
	"RECIPIENTS",
	"SERVICE_PREFERENCES",
	"WHITELIST_FILE",
	"MIN_PACKET_THRESHOLD",
	"CUSTOM_PCAP_FILTER",
	"CUSTOM_PCAP_MODE",
	"TRAINING_SESSION",
	"MANAGE_IFACE_ENABLE",
	"WEB_UI_PORT",
	"WEB_UI_IFACE",
	"CLEAR_AFTER_HOSTILE_EVENT",
	"CAPTURE_BUFFER_SIZE",
	"MASTER_UI_IP",
	"MASTER_UI_RECONNECT_TIME",
	"MASTER_UI_CLIENT_ID",
	"MASTER_UI_ENABLED",
	"MASTER_UI_PORT",
	"CLASSIFICATION_ENGINES",
	"CLASSIFICATION_CONFIGS",
	"CLASSIFICATION_MODES",
	"CLASSIFICATION_WEIGHTS",
	"ONLY_CLASSIFY_HONEYPOT_TRAFFIC",
	"CURRENT_CONFIG",
	"IPLIST_PATH",
	"EMAIL_ALERTS_ENABLED",
	"TRAINING_DATA_PATH",
	"COMMAND_START_NOVAD",
	"COMMAND_STOP_NOVAD",
	"COMMAND_START_HAYSTACK",
	"COMMAND_STOP_HAYSTACK",
	"MESSAGE_WORKER_THREADS",
	"ADDITIONAL_HONEYD_ARGS"
};

Config *Config::m_instance = NULL;

Config *Config::Inst()
{
	if(m_instance == NULL)
	{
		m_instance = new Config();
		m_instance->LoadInterfaces();
	}
	return m_instance;
}

Config::Config()
{
	pthread_rwlock_init(&m_lock, NULL);
	m_readCustomPcap = false;
	m_pathPrefix = GetEnvVariable("NOVA_PATH_PREFIX");
	if (m_pathPrefix.compare(""))
	{
		cout << "Using prefix '" + m_pathPrefix + "' on paths" << endl;
	}
	LoadPaths();

	if(!InitUserConfigs())
	{
		// Do not call LOG here, Config and logger are not yet initialized
		cout << "CRITICAL ERROR: InitUserConfigs failed" << endl;
	}
	m_readCustomPcap = false;
	m_configFilePath = m_pathHome + string("/config/NOVAConfig.txt");
	m_userConfigFilePath = m_pathHome + string("/config/settings");
	LoadUserConfig();
	LoadConfig_Internal();
	LoadVersionFile();
}

void Config::LoadCustomSettings(int argc,  char** argv)
{
	string pCAPFilePath;

	namespace po = boost::program_options;
	po::options_description desc("Command line options");
	try
	{
		desc.add_options()
				("help,h", "Show command line options")
				("pcap-file,p", po::value<string>(&pCAPFilePath), "specify Different Config Path");
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if(vm.count("help"))
		{
			std::cout << desc << std::endl;
			exit(EXIT_SUCCESS);
		}
		if(vm.count("pcap-file"))
		{
			Config::Inst()->SetPathPcapFile(pCAPFilePath);
			Config::Inst()->SetReadCustomPcap(true);
		}
	}
	catch(exception &e)
	{
		LOG(ERROR, "Uncaught exception: " + string(e.what()) + ".", "");
		std::cout << '\n' << desc << std::endl;
		exit(EXIT_FAILURE);
	}
}

void Config::LoadConfig()
{
	LoadConfig_Internal();
	LoadVersionFile();
	LoadInterfaces();
}


vector<string> Config::GetPrefixes()
{
	vector<string> ret;
	for (uint i = 0; i < sizeof(Config::Inst()->m_prefixes)/sizeof(Config::Inst()->m_prefixes[0]); i++)
	{
		ret.push_back(string(Config::Inst()->m_prefixes[i]));
	}
	return ret;
}

vector<string> Config::GetSupportedEngines()
{
	vector<string> supportedEngines;
	supportedEngines.push_back("KNN");
	supportedEngines.push_back("THRESHOLD_TRIGGER");
	supportedEngines.push_back("SCRIPT_ALERT");

	return supportedEngines;
}

// Loads the configuration file into the class's state data
void Config::LoadConfig_Internal()
{
	Lock lock(&m_lock, WRITE_LOCK);
	string line;
	string prefix;
	int prefixIndex;

	bool isValid[sizeof(m_prefixes)/sizeof(m_prefixes[0])];

	ifstream config;
	config.open(m_configFilePath.c_str());

	if(config.is_open())
	{
		while(config.good())
		{
			getline(config, line);
			prefixIndex = 0;
			prefix = m_prefixes[prefixIndex];

			// INTERFACE
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				Trim(line, ' ');
				m_interfaceLine = line;

				m_interfaces.clear();
				boost::split(m_interfaces, m_interfaceLine, boost::is_any_of("\t "), boost::token_compress_on);

				if (m_interfaces.size())
				{
					isValid[prefixIndex] = true;
				}

				continue;
			}

			// KNN_NORMALIZATION
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					vector<string> temp;
					boost::split(temp, line, boost::is_any_of(","));
					for(uint i = 0; i < temp.size(); i++)
					{
						switch(temp[i].at(0))
						{
							case '0':m_normalization.push_back(NormalizationType::NONORM);
									 break;
							case '1':m_normalization.push_back(NormalizationType::LINEAR);
									 break;
							case '2':m_normalization.push_back(NormalizationType::LINEAR_SHIFT);
									 break;
							case '3':m_normalization.push_back(NormalizationType::LOGARITHMIC);
									 break;
						}
					}
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// HS_HONEYD_CONFIG
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_pathConfigHoneydHS  = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// READ_PCAP
			prefixIndex++;
			if(this->GetCustomReadPcap() == false)
			{
				prefix = m_prefixes[prefixIndex];
				if(!line.substr(0, prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size() + 1, line.size());
					if(atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
					{
						m_readPcap = atoi(line.c_str());
						isValid[prefixIndex] = true;
					}
					continue;
				}


				// PCAP_FILE
				prefixIndex++;
				prefix = m_prefixes[prefixIndex];
				if(!line.substr(0, prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size() + 1, line.size());
					if(line.size() > 0)
					{
						m_pathPcapFile = line;
						isValid[prefixIndex] = true;
					}
					continue;
				}
			}
			else
			{
				isValid[prefixIndex] = true;
				prefixIndex++;
				isValid[prefixIndex] = true;
			}

			// CLASSIFICATION_TIMEOUT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) >= 0)
				{
					m_classificationTimeout = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// K
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) > 0)
				{
					m_k = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// EPS
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atof(line.c_str()) >= 0)
				{
					m_eps = atof(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CLASSIFICATION_THRESHOLD
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atof(line.c_str()) >= 0)
				{
					m_classificationThreshold= atof(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// DOPPELGANGER_IP
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_doppelIp = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// DOPPELGANGER_INTERFACE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_loopbackIFString = line;
					m_loopbackIF = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// DM_ENABLED
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					m_isDmEnabled = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;

			}

			// THINNING_DISTANCE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atof(line.c_str()) > 0)
				{
					m_thinningDistance = atof(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// RSYSLOG_USE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					if(!line.compare("true"))
					{
						m_rsyslogUse = true;
					}
					else
					{
						m_rsyslogUse = false;
					}
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// RSYSLOG_IP
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_rsyslogIP = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// RSYSLOG_PORT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_rsyslogPort = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// RSYSLOG_CONNTYPE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_rsyslogConnType = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// SMTP_ADDR
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					m_SMTPAddr = line;
					isValid[prefixIndex] = true;
				}

				continue;
			}

			//SMTP_PORT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					m_SMTPPort = (((in_port_t) atoi(line.c_str())));
					isValid[prefixIndex] = true;
				}

				continue;
			}

			//SMTP_DOMAIN
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					m_SMTPDomain = line;
					isValid[prefixIndex] = true;
				}

				continue;
			}

			//SMTP_INTERVAL
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					m_SMTPInterval = line;
					isValid[prefixIndex] = true;
				}

				continue;
			}

			//SMTP_PASS
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					m_SMTPPass = line;
					isValid[prefixIndex] = true;
				}

				continue;
			}

			//SMTP_USEAUTH
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					m_SMTPUseAuth = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}

				continue;
			}

			//RECIPIENTS
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					SetSMTPEmailRecipients_noLocking(line);
					isValid[prefixIndex] = true;
				}

				continue;
			}

			// SERVICE_PREFERENCES
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					m_loggerPreferences = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// WHITELIST_FILE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0 && !line.substr(line.size() - 4,
						line.size()).compare(".txt"))
				{
					m_pathWhitelistFile = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// MIN_PACKET_THRESHOLD
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_minPacketThreshold = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CUSTOM_PCAP_FILTER
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_customPcapString = line;
					isValid[prefixIndex] = true;
				}
				else
				{
					m_customPcapString = "";
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CUSTOM_PCAP_MODE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_overridePcapString = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// TRAINING_SESSION
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_trainingSession = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// MANAGE_IFACE_ENABLE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) == 0)
				{
					m_manIfaceEnable = false;
					isValid[prefixIndex] = true;
				}
				else if(atoi(line.c_str()) == 1)
				{
					m_manIfaceEnable = true;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// WEB_UI_PORT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) > 0)
				{
					m_webUIPort = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// WEB_UI_IFACE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_webUIIface = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CLEAR_AFTER_HOSTILE_EVENT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_clearAfterHostile = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CAPTURE_BUFFER_SIZE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_captureBufferSize = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// MASTER_UI_IP
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_masterUIIP = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// MASTER_UI_RECONNECT_TIME
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_masterUIReconnectTime = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// MASTER_UI_CLIENT_ID
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_masterUIClientID = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// MASTER_UI_ENABLED
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_masterUIEnabled = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// MASTER_UI_PORT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_masterUIPort = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CLASSIFICATION_ENGINES
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				isValid[prefixIndex] = true;
				if(line.size() > 0)
				{
					m_classifierEngines.clear();
					boost::split(m_classifierEngines, line, boost::is_any_of(";"));

					// Trim out any whitespace
					for (uint i = 0; i < m_classifierEngines.size(); i++)
					{
						boost::trim(m_classifierEngines[i]);
					}
				}
				continue;
			}

			// CLASSIFICATION_CONFIGS
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				isValid[prefixIndex] = true;
				if(line.size() > 0)
				{
					m_classifierConfigs.clear();
					boost::split(m_classifierConfigs, line, boost::is_any_of(";"));
				}
				continue;
			}

			// CLASSIFICATION_MODES
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				isValid[prefixIndex] = true;

				if(line.size() > 0)
				{
					vector<string> classificationTypes;
					boost::split(classificationTypes, line, boost::is_any_of(";"));

					m_classifierTypes.clear();
					if (classificationTypes.size())
					{
						for (uint i = 0; i < classificationTypes.size(); i++)
						{
							if (classificationTypes[i] == "WEIGHTED")
							{
								m_classifierTypes.push_back(CLASSIFIER_WEIGHTED);
							}
							else if (classificationTypes[i] == "HOSTILE_OVERRIDE")
							{
								m_classifierTypes.push_back(CLASSIFIER_HOSTILE_OVERRIDE);
							}
							else if (classificationTypes[i] == "BENIGN_OVERRIDE")
							{
								m_classifierTypes.push_back(CLASSIFIER_BENIGN_OVERRIDE);
							}
							else
							{
								LOG(ERROR, "Bad classifier type in config file: " + classificationTypes[i], "");
								isValid[prefixIndex] = false;
								continue;
							}
						}

					}

				}
				continue;
			}

			// CLASSIFICATION_WEIGHTS
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				isValid[prefixIndex] = true;
				if(line.size() > 0)
				{
					m_classifierWeights.clear();
					vector<string> weights;
					boost::split(weights, line, boost::is_any_of(";"));

					if (weights.size())
					{
						for (uint i = 0; i < weights.size(); i++)
						{
							m_classifierWeights.push_back(atoi(weights[i].c_str()));
						}

					}
				}
				continue;
			}

			// ONLY_CLASSIFY_HONEYPOT_TRAFFIC
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_onlyClassifyHoneypotTraffic = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
			}

			// CURRENT_CONFIG
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_currentConfig = string(line.c_str());
					isValid[prefixIndex] = true;
				}
			}

			// IPLIST_PATH
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_iplistPath = string(line.c_str());
					isValid[prefixIndex] = true;
				}
			}

			// EMAIL_ALERTS_ENABLED
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_emailAlertsEnabled = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
			}

			// TRAINING_DATA_PATH
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_pathTrainingData = line;
					isValid[prefixIndex] = true;
				}
			}

			// COMMAND_START_NOVAD
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_commandStartNovad = line;
					isValid[prefixIndex] = true;
				}
			}

			// COMMAND_STOP_NOVAD
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_commandStopNovad = line;
					isValid[prefixIndex] = true;
				}
			}

			// COMMAND_START_HAYSTACK
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_commandStartHaystack = line;
					isValid[prefixIndex] = true;
				}
			}

			// COMMAND_STOP_HAYSTACK
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_commandStopHaystack = line;
					isValid[prefixIndex] = true;
				}
			}

			//MESSAGE_WORKER_THREADS
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					m_messageWorkerThreads = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}

				continue;
			}

			//ADDITIONAL_HONEYD_ARGS
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				m_additionalHoneydArgs = line;
				isValid[prefixIndex] = true;

				continue;
			}
		}
	}
	else
	{
		// Do not call LOG here, Config and Logger are not yet initialized
		cout << "CRITICAL ERROR: No configuration file found! Could not open: " << m_configFilePath << endl;
		exit(EXIT_FAILURE);
	}

	config.close();

	bool failAndExit = false;
	for(uint i = 0; i < sizeof(m_prefixes)/sizeof(m_prefixes[0]); i++)
	{
		if(!isValid[i])
		{
			failAndExit = true;
			stringstream ss;
			ss << "File: " << __FILE__ << " at line " << __LINE__ << ": Configuration option '"
				<< m_prefixes[i] << "' is invalid.";
			::openlog("Nova", OPEN_SYSL, LOG_AUTHPRIV);
			syslog(ERROR, "%s %s", "ERROR", ss.str().c_str());
			closelog();
		}
	}

	if (failAndExit)
	{
		exit(EXIT_FAILURE);
	}
}

bool Config::LoadUserConfig()
{
	Lock lock(&m_lock, WRITE_LOCK);

	string prefix, line;
	uint i = 0;
	bool returnValue = true;
	ifstream settings(m_userConfigFilePath.c_str());

	if(settings.is_open())
	{
		while(settings.good())
		{
			getline(settings, line);
			i++;

			prefix = "group";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				m_group = line;
				continue;
			}
		}
	}
	else
	{
		returnValue = false;
	}
	settings.close();
	return returnValue;
}

bool Config::LoadPaths()
{
	Lock lock(&m_lock, WRITE_LOCK);

	// Try getting the $HOME env var
	char *homePath = getenv("HOME");
	if (homePath != NULL)
	{
		m_pathHome = string(homePath);
	}
	// Resort to checking the passwd file
	else
	{
		struct passwd *pw = getpwuid(getuid());
		m_pathHome = string(pw->pw_dir);
	}

	m_pathHomeConfig = m_pathHome + "/.config";
	m_pathHome += "/.config/nova";

	//Get locations of nova files
	ifstream *paths =  new ifstream(m_pathPrefix + Config::m_pathsFile);
	string prefix, line;

	if(paths->is_open())
	{
		while(paths->good())
		{
			getline(*paths,line);

			prefix = "NOVA_RD";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				m_pathReadFolder = m_pathPrefix + ResolvePathVars(line);
				continue;
			}

			prefix = "NOVA_ICON";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				m_pathIcon = m_pathPrefix + ResolvePathVars(line);
				continue;
			}
		}
	}
	paths->close();
	delete paths;
	return true;
}

void Config::LoadInterfaces()
{
	Lock lock(&m_lock, WRITE_LOCK);
	struct ifaddrs *devices = NULL;
	ifaddrs *curIf = NULL;
	stringstream ss;

	//Get a list of interfaces
	if(getifaddrs(&devices))
	{
		LOG(ERROR, string("Ethernet Interface Auto-Detection failed: ") + string(strerror(errno)), "");
	}

	//Choose the first loopback adapter in the default case
	if(!m_loopbackIFString.compare("default"))
	{
		m_loIsDefault = true;
		for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
		{
			//If we find a valid loopback interface exit the loop early (curIf != NULL)
			if(curIf->ifa_addr != NULL && (curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_INET))
			{
				m_loopbackIF = string(curIf->ifa_name);
				break;
			}
		}
	}
	else
	{
		m_loIsDefault = false;
	}

	m_interfaces.clear();
	boost::split(m_interfaces, m_interfaceLine, boost::is_any_of("\t "), boost::token_compress_on);
	vector<string> interfaces = m_interfaces;
	//Use all valid devices
	if(!interfaces[0].compare("default"))
	{
		m_interfaces.clear();
		m_ifIsDefault = true;
		for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
		{
			if(curIf->ifa_addr != NULL && !(curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_INET))
			{
				m_interfaces.push_back(string(curIf->ifa_name));
			}
		}
	}
	else
	{
		m_ifIsDefault = false;
	}

	freeifaddrs(devices);
}

bool Config::LoadVersionFile()
{
	ifstream versionFile((m_pathHome + "/config/" + VERSION_FILE_NAME).c_str());
	string line;

	if(versionFile.is_open())
	{
		if(versionFile.good())
		{
			getline(versionFile, line);
			string temp = line.substr(line.find_first_of(".") + 1, string::npos);

			m_version.versionString = line;
			m_version.buildYear = atoi(line.substr(0, line.find_first_of(".")).c_str());
			m_version.buildMonth = atoi(temp.substr(0, temp.find_first_of(".")).c_str());

			if (temp.find_first_of(".") != string::npos)
			{
				m_version.minorVersion = atoi(temp.substr(temp.find_first_of(".") + 1, string::npos).c_str());
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}

version Config::GetVersion()
{
	return m_version;
}

string Config::GetVersionString()
{
	return m_version.versionString;
}

string Config::ResolvePathVars(string path)
{
	int start = 0;
	int end = 0;
	string var = "";

	while((start = path.find("$",end)) != -1)
	{
		end = path.find("/", start);
		//If no path after environment var
		if(end == -1)
		{

			var = path.substr(start+1, path.size());
			var = getenv(var.c_str());
			path = path.substr(0,start) + var;
		}
		else
		{
			var = path.substr(start+1, end-1);
			var = getenv(var.c_str());
			var = var + path.substr(end, path.size());
			if(start > 0)
			{
				path = path.substr(0,start)+var;
			}
			else
			{
				path = var;
			}
		}
	}
	if(var.compare(""))
	{
		return var;
	}
	else
	{
		return path;
	}
}

//	Returns: True if(after the function) the user has all necessary nova config files
//		IE: Returns false only if the user doesn't have configs AND we weren't able to make them
bool Config::InitUserConfigs()
{
	bool returnValue = true;
	struct stat fileAttr;

	// Important note
	// This is called before the logger is initialized. Calling LOG here will likely result in a crash. Just use cout instead.

	//check for home folder
	if(!stat(m_pathHome.c_str(), &fileAttr ) == 0)
	{
		boost::filesystem::create_directories(m_pathHomeConfig);
		boost::filesystem::path fromPath = m_pathPrefix + "/usr/share/nova/userFiles/";
		boost::filesystem::path toPath = m_pathHome + "/";

		if(!RecursiveDirectoryCopy(fromPath, toPath, false))
		{
			cout << "Error copying files to user's HOME folder." << endl;
		}

		if(stat(m_pathHome.c_str(), &fileAttr) == 0)
		{

			// Ugh terrible terrible hack to let honeyd scripts access this file as the nobody user
			// TODO: Why do we not have global configuration/state files in /var,/etc, and /tmp like every other daemon application?
			if (system("chmod o+rwx ~/.config/nova")) {}
			if (system("chmod o+rwx ~/.config/nova/data")) {}
			if (system("chmod o+rw ~/.config/nova/data/scriptAlerts.db")) {}

			return returnValue;
		}
		else
		{
			cout << "Was unable to create directory " << m_pathHome << endl;
			returnValue = false;
		}
	}

	return returnValue;
}

std::string Config::ReadSetting(std::string key)
{
	Lock lock(&m_lock, WRITE_LOCK);

	string line;
	string value;

	ifstream config(m_configFilePath.c_str());

	if(config.is_open())
	{
		while(config.good())
		{
			getline(config, line);

			if(!line.substr(0, key.size() + 1).compare(key + " "))
			{
				line = line.substr(key.size() + 1, line.size());
				if(line.size() > 0)
				{
					value = line;
					break;
				}
				continue;
			}
		}

		config.close();

		return value;
	}
	else
	{
		LOG(ERROR, "Unable to read configuration file", "");

		return "";
	}
}

bool Config::WriteSetting(std::string key, std::string value)
{
	Lock lock(&m_lock, WRITE_LOCK);

	bool validKey = false;
	for (uint i = 0; i < sizeof(Config::Inst()->m_prefixes)/sizeof(Config::Inst()->m_prefixes[0]); i++)
	{
		if (!Config::Inst()->m_prefixes[i].compare(key))
		{
			validKey = true;
			break;
		}
	}

	if (!validKey)
	{
		LOG(WARNING, "WriteSetting was called with invalid setting key of " + key, "");
		return false;
	}

	string line;
	bool error = false;

	//Rewrite the config file with the new settings
	string configurationBackup = m_configFilePath + ".tmp";
	boost::filesystem::path from = m_configFilePath;
	boost::filesystem::path to = configurationBackup;
	string copyCommand = "cp -fp " + m_configFilePath + " " + configurationBackup;
	boost::filesystem::copy_file(from, to, boost::filesystem::copy_option::overwrite_if_exists);

	ifstream *in = new ifstream(configurationBackup.c_str());
	ofstream *out = new ofstream(m_configFilePath.c_str());

	if(out->is_open() && in->is_open())
	{
		while(in->good())
		{
			if(!getline(*in, line))
			{
				continue;
			}


			if(!line.substr(0,key.size() + 1).compare(key + " "))
			{
				*out << key << " " << value << endl;
				continue;
			}

			*out << line << endl;
		}
	}
	else
	{
		error = true;
	}

	in->close();
	out->close();
	delete in;
	delete out;

	boost::filesystem::remove(to);

	if(error)
	{
		LOG(ERROR, "Problem saving current configuration.", "");
		return false;
	}
	else
	{
		return true;
	}
}

bool Config::GetUseAllInterfaces()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_ifIsDefault;
}

bool Config::GetUseAnyLoopback()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_loIsDefault;
}

string Config::GetInterface(uint i)
{
	Lock lock(&m_lock, READ_LOCK);
	string interface = "";
	if(m_interfaces.size() > i)
	{
		interface = m_interfaces[i];
	}
	return interface;
}

vector<string> Config::GetInterfaces()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_interfaces;
}

vector<string> Config::GetIPv4HostInterfaceList()
{
	Lock lock(&m_lock, READ_LOCK);
	struct ifaddrs *devices = NULL;
	ifaddrs *curIf = NULL;
	vector<string> ret;
	//Get a list of interfaces
	if(getifaddrs(&devices))
	{
		LOG(ERROR, string("Ethernet Interface Auto-Detection failed: ") + string(strerror(errno)), "");
	}
	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		if(curIf->ifa_addr != NULL && ((int)curIf->ifa_addr->sa_family == AF_INET) && !(curIf->ifa_flags & IFF_LOOPBACK))
		{
			ret.push_back(curIf->ifa_name);
		}
	}
	return ret;
}

vector<string> Config::GetIPv4LoopbackInterfaceList()
{
	Lock lock(&m_lock, READ_LOCK);
	struct ifaddrs *devices = NULL;
	ifaddrs *curIf = NULL;
	vector<string> ret;
	//Get a list of interfaces
	if(getifaddrs(&devices))
	{
		LOG(ERROR, string("Ethernet Interface Auto-Detection failed: ") + string(strerror(errno)), "");
	}
	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		if(curIf->ifa_addr != NULL && ((int)curIf->ifa_addr->sa_family == AF_INET) && (curIf->ifa_flags & IFF_LOOPBACK))
		{
			ret.push_back(curIf->ifa_name);
		}
	}
	return ret;
}

uint Config::GetInterfaceCount()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_interfaces.size();
}

void Config::SetUseAllInterfaces(bool which)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_ifIsDefault = which;
}

bool Config::SetInterfaces(std::vector<std::string> newInterfaceList)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_interfaces = newInterfaceList;
	return true;
}

void Config::SetUseAnyLoopback(bool which)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_loIsDefault = which;
}

bool Config::SetUseAllInterfacesBinding(bool which)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_ifIsDefault = which;
	return true;
}

bool Config::SetUseAnyLoopbackBinding(bool which)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_loIsDefault = which;
	return true;
}

void Config::AddInterface(string interface)
{
	Lock lock(&m_lock, WRITE_LOCK);
	for(uint i = 0; i < m_interfaces.size(); i++)
	{
		if(!m_interfaces[i].compare(interface))
		{
			return;
		}
	}
	m_interfaces.push_back(interface);
}

void Config::RemoveInterface(string interface)
{
	Lock lock(&m_lock, WRITE_LOCK);
	for(uint i = 0; i < m_interfaces.size(); i++)
	{
		if(!m_interfaces[i].compare(interface))
		{
			m_interfaces.erase(m_interfaces.begin()+i);
			break;
		}
	}
}

void Config::ClearInterfaces()
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_interfaces.clear();
}

std::vector<std::string> Config::ListInterfaces()
{
	Lock lock(&m_lock, WRITE_LOCK);
	struct ifaddrs * devices = NULL;
	ifaddrs *curIf = NULL;
	stringstream ss;

	//Get a list of interfaces
	if(getifaddrs(&devices))
	{
		LOG(ERROR, string("Ethernet Interface Auto-Detection failed: ") + string(strerror(errno)), "");
	}

	// ********** ETHERNET INTERFACES ************* //
	vector<string> interfaces;

	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		if(curIf->ifa_addr != NULL && !(curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_PACKET))
		{
			interfaces.push_back(string(curIf->ifa_name));
		}
	}

	freeifaddrs(devices);
	return interfaces;
}

std::string Config::GetUseAllInterfacesBinding()
{
	if(m_ifIsDefault)
	{
		return "true";
	}
	else
	{
		return "false";
	}
}

std::vector<std::string> Config::ListLoopbacks()
{
	Lock lock(&m_lock, WRITE_LOCK);
	struct ifaddrs * devices = NULL;
	ifaddrs *curIf = NULL;
	stringstream ss;
	std::vector<std::string> ret;

	//Get a list of interfaces
	if(getifaddrs(&devices))
	{
		LOG(ERROR, string("Ethernet Interface Auto-Detection failed: ") + string(strerror(errno)), "");
	}

	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		//If we find a valid loopback interface exit the loop early (curIf != NULL)
		if(curIf->ifa_addr != NULL && (curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_INET))
		{
			ret.push_back(string(curIf->ifa_name));
		}
	}

	freeifaddrs(devices);
	return ret;
}

void Config::SetSMTPEmailRecipients_noLocking(string SMTPEmailRecipients)
{
	vector<string> addresses;
	istringstream iss(SMTPEmailRecipients);
	string token;
	while(getline(iss, token, ','))
	{
		addresses.push_back(token);
	}

	m_SMTPEmailRecipients = addresses;
}

string Config::GetPathReadFolder()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_pathReadFolder;
}

string Config::GetPathHome()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_pathHome;
}

vector <string> Config::GetIpAddresses(string ipListFile)
{
	ifstream ipListFileStream(ipListFile.c_str());
	vector<string> whitelistedAddresses;

	if(ipListFileStream.is_open())
	{
		while(ipListFileStream.good())
		{
			string line;
			getline(ipListFileStream,line);
			if(line != "" && line.at(0) != '#' )
			{
				whitelistedAddresses.push_back(line);
			}
		}
		ipListFileStream.close();
	}
	else
	{
		LOG(ERROR,"Unable to open file: " + ipListFile, "");
	}

	return whitelistedAddresses;
}

vector <HoneypotAddress> Config::GetHoneydIpAddresses(string ipListFile)
{
	ifstream ipListFileStream(ipListFile.c_str());
	vector<HoneypotAddress> addresses;

	if(ipListFileStream.is_open())
	{
		while(ipListFileStream.good())
		{
			string line;
			getline(ipListFileStream,line);
			if(line != "" && line.at(0) != '#' )
			{
				std::vector<std::string> strs;
				boost::split(strs, line, boost::is_any_of(","));
				if (strs.size() >= 3)
				{
					HoneypotAddress t;
					t.ip = strs.at(0);
					t.interface = strs.at(2);

					boost::trim(t.ip);
					boost::trim(t.interface);

					addresses.push_back(t);
				}
				else if (strs.size() == 2)
				{
					LOG(CRITICAL, "Honeyd IP list file doesn't contain network interfaces! Please update to the newest version of Honeyd.", "");
				}
			}
		}
		ipListFileStream.close();
	}
	else
	{
		LOG(ERROR,"Unable to open file: " + ipListFile, "");
	}

	return addresses;
}


// Finds the interface who's subnet this IP falls into
string Config::findAssociatedInterface(string ipString) {
	// Find out which interface this IP is in
	struct in_addr ip;
	inet_aton(ipString.c_str(), &ip);

	struct ifaddrs *devices = NULL;
	ifaddrs *curIf = NULL;
	getifaddrs(&devices);
	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		if(curIf->ifa_addr != NULL && ((int)curIf->ifa_addr->sa_family == AF_INET))
		{
			uint32_t match1 = ((sockaddr_in*)(curIf->ifa_addr))->sin_addr.s_addr & ((sockaddr_in*)(curIf->ifa_netmask))->sin_addr.s_addr;
			uint32_t match2 = ip.s_addr & ((sockaddr_in*)(curIf->ifa_netmask))->sin_addr.s_addr;

			if (match1 == match2) {
				return string(curIf->ifa_name);
			}
		}
	}
	return "";
}

vector <HoneypotAddress> Config::GetHaystackAddresses(string honeyDConfigPath)
{
	//Path to the main log file
	ifstream honeydConfFile(honeyDConfigPath.c_str());
	vector<HoneypotAddress> retAddresses;

	if( honeydConfFile == NULL)
	{
		LOG(ERROR, string("Error opening haystack file at ") + honeyDConfigPath + ". Does it exist?", "");
		exit(EXIT_FAILURE);
	}

	string LogInputLine;

	while(!honeydConfFile.eof())
	{
		stringstream LogInputLineStream;

		//Get the next line
		getline(honeydConfFile, LogInputLine);

		//Load the line into a stringstream for easier tokenizing
		LogInputLineStream << LogInputLine;
		string token;
		string honeydTemplate;

		//Is the first word "bind"?
		getline(LogInputLineStream, token, ' ');

		if(token.compare( "bind" ) != 0)
		{
			continue;
		}

		//The next token will be the IP address
		getline(LogInputLineStream, token, ' ');

		// Get the template
		getline(LogInputLineStream, honeydTemplate, ' ');

		if(honeydTemplate != "DoppelgangerReservedTemplate")
		{
			HoneypotAddress address;
			address.ip = token;
			address.interface = findAssociatedInterface(address.ip);

			// Find the interface for this token
			retAddresses.push_back(address);
		}
	}
	return retAddresses;
}


}
