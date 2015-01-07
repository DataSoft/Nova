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
//============================================================================/*

#ifndef NOVACONFIGURATION_H_
#define NOVACONFIGURATION_H_

#include "HashMapStructs.h"
#include "ClassificationEngine.h"
#include "Lock.h"


#define MAKE_GETTER_SETTER(type,name,getName,setName) \
    type name; \
    type getName()          {Nova::Lock lock(&m_lock, READ_LOCK); return name;} \
    bool setName(type name) {Nova::Lock lock(&m_lock, WRITE_LOCK); this->name = name; return true;}

#define MAKE_GETTER(type,name,getName) \
    type name; \
    type getName()          {Nova::Lock lock(&m_lock, READ_LOCK); return name;}

namespace Nova
{

#define VERSION_FILE_NAME "version.txt"

struct version
{
	std::string versionString;
	int buildYear;
	int buildMonth;
	int minorVersion;

	version()
	{
		versionString = "VERSION NOT SET";
		buildYear = 0;
		buildMonth = 0;
		minorVersion = 0;
	}
};

enum CLASSIFIER_MODES {
	CLASSIFIER_WEIGHTED,
	CLASSIFIER_HOSTILE_OVERRIDE,
	CLASSIFIER_BENIGN_OVERRIDE
};

enum NormalizationType {
	NONORM, 		// Does no data normalization. Feature must already be in a range from 0 to 1
	LINEAR,			// Simple linear normalization by dividing data by the max
	LINEAR_SHIFT, 	// Shifts min value to 0 before doing linear normalization
	LOGARITHMIC		// Logarithmic normalization, larger outlier value will have less of an effect
};

struct HoneypotAddress {
	HoneypotAddress(std::string ip, std::string interface) {
		this->ip = ip;
		this->interface = interface;
	}
	HoneypotAddress() {}

	std::string ip;
	std::string interface;
};

class Config
{

public:
	// This is a singleton class, use this to access it
	static Config *Inst();

	// Loads and parses a NOVA configuration file
	//      module - added s.t. rsyslog  will output NovaConfig messages as the parent process that called LoadConfig
	void LoadConfig();
	void LoadCustomSettings(int argc,  char** argv);

	// Checks to see if the current user has a ~/.nova directory, and creates it if not, along with default config files
	//	Returns: True if(after the function) the user has all necessary ~/.nova config files
	//		IE: Returns false only if the user doesn't have configs AND we weren't able to make them
    bool InitUserConfigs();

    // These are generic static getters/setters for the web interface
    // Use of these should be minimized. Instead, use the specific typesafe getter/setter
    // that you need.
    std::string ReadSetting(std::string key);
    bool WriteSetting(std::string key, std::string value);

    std::string GetInterface(uint i);
    std::vector<std::string> GetInterfaces();
    std::vector<std::string> GetIPv4HostInterfaceList();
    std::vector<std::string> GetIPv4LoopbackInterfaceList();
    uint GetInterfaceCount();

    bool GetUseAllInterfaces();
    bool GetUseAnyLoopback();
    std::string GetUseAllInterfacesBinding();

    void AddInterface(std::string interface);
    void RemoveInterface(std::string interface);
    void ClearInterfaces();
    std::vector<std::string> ListInterfaces();
    std::vector<std::string> ListLoopbacks();
    bool SetInterfaces(std::vector<std::string> newInterfaceList);
    void SetUseAllInterfaces(bool which);
    void SetUseAnyLoopback(bool which);
    bool SetUseAllInterfacesBinding(bool which);
    bool SetUseAnyLoopbackBinding(bool which);

	bool GetSMTPSettings_FromFile();
	bool SaveSMTPSettings();

	// Set with a CSV std::string from the config file
    void SetSMTPEmailRecipients(std::string SMTPEmailRecipients);

    // Getters for the paths stored in /etc
    std::string GetPathReadFolder();
    std::string GetPathHome();

    inline std::string GetPathShared() {
    	return m_pathPrefix + "/usr/share/nova/sharedFiles";
    }


	version GetVersion();
	std::string GetVersionString();

	static std::string findAssociatedInterface(std::string ip);
	static std::vector <HoneypotAddress> GetHaystackAddresses(std::string honeyDConfigPath);
	static std::vector <HoneypotAddress> GetHoneydIpAddresses(std::string ipListFile);

	// TODO is this used anymore?
	static std::vector <std::string> GetIpAddresses(std::string ipListFile);

	std::vector<std::string> GetPrefixes();

	//Attempts to detect and use intefaces returned by pcap_lookupdev
	void LoadInterfaces();

	std::vector<std::string> GetSupportedEngines();

	MAKE_GETTER_SETTER(int, m_k, GetK, SetK);
	MAKE_GETTER_SETTER(std::string, m_configFilePath, GetConfigFilePath, SetConfigFilePath);

	MAKE_GETTER(std::string, m_commandStartNovad, GetCommandStartNovad);
	MAKE_GETTER(std::string, m_commandStopNovad, GetCommandStopNovad);
	MAKE_GETTER(std::string, m_commandStartHaystack, GetCommandStartHaystack);
	MAKE_GETTER(std::string, m_commandStopHaystack, GetCommandStopHaystack);

	MAKE_GETTER(std::vector<std::string>, m_classifierEngines, GetClassificationEngines);
	MAKE_GETTER(std::vector<std::string>, m_classifierConfigs, GetClassifierConfigs);
	MAKE_GETTER(std::vector<CLASSIFIER_MODES>, m_classifierTypes, GetClassifierModes);
	MAKE_GETTER(std::vector<int>, m_classifierWeights, GetClassifierWeights);
	MAKE_GETTER(std::vector<NormalizationType>, m_normalization, GetNormalizationFunctions);

	MAKE_GETTER(bool, m_onlyClassifyHoneypotTraffic, GetOnlyClassifyHoneypotTraffic);
	MAKE_GETTER(std::string, m_pathIcon, GetPathIcon);

	MAKE_GETTER_SETTER(double, m_classificationThreshold, GetClassificationThreshold, SetClassificationThreshold);
	MAKE_GETTER_SETTER(int, m_classificationTimeout, GetClassificationTimeout, SetClassificationTimeout);
	MAKE_GETTER_SETTER(std::string, m_loopbackIF, GetDoppelInterface, SetDoppelInterface);
	MAKE_GETTER_SETTER(double, m_eps, GetEps, SetEps);
	MAKE_GETTER_SETTER(bool, m_isDmEnabled, GetIsDmEnabled, SetIsDmEnabled);
	MAKE_GETTER_SETTER(std::string, m_pathConfigHoneydUser, GetPathConfigHoneydUser , SetPathConfigHoneydUser);
	MAKE_GETTER_SETTER(std::string, m_pathConfigHoneydHS, GetPathConfigHoneydHS, SetPathConfigHoneydHS);
	MAKE_GETTER_SETTER(std::string, m_pathPcapFile, GetPathPcapFile, SetPathPcapFile);
	MAKE_GETTER_SETTER(std::string, m_pathWhitelistFile, GetPathWhitelistFile, SetPathWhitelistFile);
	MAKE_GETTER_SETTER(bool, m_readPcap, GetReadPcap, SetReadPcap);
	MAKE_GETTER_SETTER(bool, m_readCustomPcap, GetCustomReadPcap, SetReadCustomPcap);
	MAKE_GETTER_SETTER(double, m_thinningDistance, GetThinningDistance, SetThinningDistance);
	MAKE_GETTER_SETTER(std::string, m_group, GetGroup, SetGroup);
	MAKE_GETTER_SETTER(std::string, m_currentConfig, GetCurrentConfig, SetCurrentConfig);
	MAKE_GETTER_SETTER(std::string, m_doppelIp, GetDoppelIp, SetDoppelIp);
	MAKE_GETTER_SETTER(std::string, m_iplistPath, GetIpListPath, SetIpListPath);

	MAKE_GETTER_SETTER(std::string, m_loggerPreferences, GetLoggerPreferences, SetLoggerPreferences);

	// the SMTP server domain name for display purposes
	MAKE_GETTER_SETTER(std::string, m_SMTPDomain, GetSMTPDomain, SetSMTPDomain);
	// cron interval for SMTP alerts
	MAKE_GETTER_SETTER(std::string, m_SMTPInterval, GetSMTPInterval, SetSMTPInterval);
	// the email address that will be set as sender
	MAKE_GETTER_SETTER(std::string, m_SMTPAddr, GetSMTPAddr, SetSMTPAddr);
	// the port for SMTP send; normally 25 if I'm not mistaken, 465 for SSL and 5 hundred something for TLS
	MAKE_GETTER_SETTER(in_port_t, m_SMTPPort, GetSMTPPort, SetSMTPPort);
	MAKE_GETTER_SETTER(bool, m_SMTPUseAuth, GetSMTPUseAuth, SetSMTPUseAuth);

	// password for interacting with the SMTP account that acts
	// as the relay for Nova mail alerts
	MAKE_GETTER_SETTER(std::string, m_SMTPPass, GetSMTPPass, SetSMTPPass);
	MAKE_GETTER_SETTER(std::vector<std::string>, m_SMTPEmailRecipients, GetSMTPEmailRecipients, SetSMTPEmailRecipients);

	MAKE_GETTER_SETTER(bool, m_rsyslogUse, UseRsyslog, SetUseRsyslog);
	MAKE_GETTER_SETTER(std::string, m_rsyslogIP, GetRsyslogIP, SetRsyslogIP);
	MAKE_GETTER_SETTER(std::string, m_rsyslogPort, GetRsyslogPort, SetRsyslogPort);
	MAKE_GETTER_SETTER(std::string, m_rsyslogConnType, GetRsyslogConnType, SetRsyslogConnType);

	MAKE_GETTER_SETTER(uint, m_minPacketThreshold , GetMinPacketThreshold, SetMinPacketThreshold);
	MAKE_GETTER_SETTER(std::string, m_customPcapString , GetCustomPcapString, SetCustomPcapString);
	MAKE_GETTER_SETTER(bool, m_overridePcapString, GetOverridePcapString, SetOverridePcapString);
	MAKE_GETTER_SETTER(std::string, m_trainingSession , GetTrainingSession, SetTrainingSession);
	MAKE_GETTER_SETTER(int, m_webUIPort , GetWebUIPort, SetWebUIPort);

	MAKE_GETTER_SETTER(bool, m_clearAfterHostile, GetClearAfterHostile, SetClearAfterHostile);
	MAKE_GETTER_SETTER(int, m_captureBufferSize, GetCaptureBufferSize, SetCaptureBufferSize);
	MAKE_GETTER_SETTER(int, m_masterUIPort, GetMasterUIPort, SetMasterUIPort);
	MAKE_GETTER_SETTER(bool, m_emailAlertsEnabled, GetAreEmailAlertsEnabled, SetAreEmailAlertsEnabled);
	MAKE_GETTER_SETTER(std::string,m_pathTrainingData, GetPathTrainingData, SetPathTrainingData);

	MAKE_GETTER_SETTER(int, m_messageWorkerThreads, GetNumMessageWorkerThreads, SetNumMEssageWorkerThreads);
	MAKE_GETTER_SETTER(std::string, m_additionalHoneydArgs, GetAdditionalHoneydArgs, SetAdditionalHoneydArgs);

protected:
	Config();

private:
	static Config *m_instance;

	__attribute__ ((visibility ("hidden"))) static std::string m_prefixes[];

	std::string m_loopbackIFString;
	bool m_loIsDefault;
	bool m_ifIsDefault;

	// List of currently used interfaces
	std::vector<std::string> m_interfaces;


	// What the actual config file contains
	std::string m_interfaceLine;

	int m_tcpTimout;
	int m_tcpCheckFreq;
	bool m_manIfaceEnable;
	std::string m_webUIIface;

	version m_version;

	static std::string m_pathsFile;

	std::string m_userConfigFilePath;

	// Options from the PATHS file (currently /etc/nova/paths)
	std::string m_pathReadFolder;
	std::string m_pathHomeConfig;
	std::string m_pathHome;


	bool m_masterUIEnabled;
	int m_masterUIReconnectTime;
	std::string m_masterUIIP;
	std::string m_masterUIClientID;
	static std::string m_pathPrefix;

	pthread_rwlock_t m_lock;

	// Used for loading the nova path file, resolves paths with env vars to full paths
	static std::string ResolvePathVars(std::string path);

    // Set with a CSV std::string from the config file
    void SetSMTPEmailRecipients_noLocking(std::string SMTPEmailRecipients);

	// Loads the PATH file (usually in /etc)
	bool LoadPaths();

	// Loads the version file
	bool LoadVersionFile();


	bool LoadUserConfig();

	//Private version of LoadConfig so the public version can call LoadInterfaces()
	//	LoadInterfaces cannot be called until m_instance has been created, but needs to execute after every load
	//	However the constructor calls LoadConfig, so we use a private version instead that doesn't include
	//	LoadInterfaces() which is called elsewhere
	void LoadConfig_Internal();

};
}

#endif /* NOVACONFIGURATION_H_ */
