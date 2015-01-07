#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include <iostream>
#include <string>
#include <map>
#include "HoneydProfileBinding.h"
#include "HoneydConfiguration/HoneydConfiguration.h"

using namespace std;
using namespace v8;
using namespace Nova;

HoneydProfileBinding::HoneydProfileBinding(string parentName, string profileName)
{
	m_profile = new Profile(parentName, profileName);
}

HoneydProfileBinding::~HoneydProfileBinding()
{
}

bool HoneydProfileBinding::SetPersonality(std::string personality)
{
	m_profile->SetPersonality(personality);
	return true;
}

Handle<Value> HoneydProfileBinding::AddProxy(const Arguments& args)
{
	HandleScope scope;
	
	HoneydProfileBinding* obj = ObjectWrap::Unwrap<HoneydProfileBinding>(args.This());

	if (args.Length() != 4) {
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 4 parameters")));
	}
	
	Profile *profile = obj->m_profile;


	Proxy *proxy = new Proxy();
	proxy->m_protocol = cvv8::CastFromJS<string>(args[0]);
	proxy->m_honeypotPort = cvv8::CastFromJS<int>(args[1]);
	proxy->m_proxyIP = cvv8::CastFromJS<string>(args[2]);
	proxy->m_proxyPort = cvv8::CastFromJS<int>(args[3]);

	profile->m_proxies.push_back(proxy);

	return scope.Close( Null() );
}

Handle<Value> HoneydProfileBinding::AddBroadcast(const Arguments& args)
{
	HandleScope scope;
	
	HoneydProfileBinding* obj = ObjectWrap::Unwrap<HoneydProfileBinding>(args.This());

	if (args.Length() != 4) {
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 4 parameters")));
	}
	
	std::string scriptName = cvv8::CastFromJS<string>(args[0]);
	int srcPort  = cvv8::CastFromJS<int>(args[1]);
	int dstPort = cvv8::CastFromJS<int>(args[2]);
	int time = cvv8::CastFromJS<int>(args[3]);

	Profile *profile = obj->m_profile;

	Broadcast *bcast = new Broadcast();
	bcast->m_srcPort = srcPort;
	bcast->m_dstPort = dstPort;
	bcast->m_script = scriptName;
	bcast->m_time = time;

	profile->m_broadcasts.push_back(bcast);

	return scope.Close( Null() );
}

bool HoneydProfileBinding::ClearBroadcasts()
{
	for (uint i = 0; i < m_profile->m_broadcasts.size(); i++)
	{
		delete m_profile->m_broadcasts[i];
	}

	m_profile->m_broadcasts.clear();	
	return true;
}

bool HoneydProfileBinding::ClearProxies()
{
	for (uint i = 0; i < m_profile->m_proxies.size(); i++)
	{
		delete m_profile->m_proxies[i];
	}

	m_profile->m_proxies.clear();	
	return true;
}

bool HoneydProfileBinding::SetCount(int count)
{
	m_profile->m_count = count;
	return true;
}

Profile *HoneydProfileBinding::GetChild()
{
	return m_profile;
}

Handle<Value> HoneydProfileBinding::SetVendors(const Arguments& args)
{
	HandleScope scope;
	if( args.Length() != 2 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with two parameters")));
	}

	HoneydProfileBinding* obj = ObjectWrap::Unwrap<HoneydProfileBinding>(args.This());


	vector<string> vendorNames = cvv8::CastFromJS<vector<string>>( args[0] );
	vector<uint> vendorCount = cvv8::CastFromJS<vector<uint>>( args[1] );

	if(vendorNames.size() != vendorCount.size())
	{
		//Mismatch in sizes
		return scope.Close(Boolean::New(false));
	}

	if(vendorNames.size() == 0)
	{
		obj->m_profile->m_vendors.clear();
		return scope.Close(Boolean::New(true));
	}
	else
	{
		std::vector<std::pair<std::string, uint> > set;

		for(uint i = 0; i < vendorNames.size(); i++)
		{
			std::pair<std::string, uint> vendor;
			vendor.first = vendorNames[i];
			vendor.second = vendorCount[i];
			set.push_back(vendor);
		}
		obj->m_profile->m_vendors = set;
		return scope.Close(Boolean::New(true));
	}
}

Handle<Value> HoneydProfileBinding::AddPort(const Arguments& args)
{
	if( args.Length() != 7 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with seven parameters")));
	}

	HandleScope scope;
	HoneydProfileBinding* obj = ObjectWrap::Unwrap<HoneydProfileBinding>(args.This());



	int portSetIndex = cvv8::CastFromJS<int>( args[0] );
	string portBehavior = cvv8::CastFromJS<string>( args[1] );
	string portProtcol = cvv8::CastFromJS<string>( args[2] );
	uint portNumber = cvv8::CastFromJS<uint>( args[3] );
	string portScriptName = cvv8::CastFromJS<string>( args[4] );
	vector<string> scriptConfigurationKeys = cvv8::CastFromJS<vector<string>>( args[5] );
	vector<string> scriptConfigurationValues = cvv8::CastFromJS<vector<string>>( args[6] );

	PortSet *portSet = obj->m_profile->GetPortSet(portSetIndex);
	if(portSet == NULL)
	{
		cout << "ERROR: Unable to get portset " << portSetIndex << endl;
		return scope.Close(Boolean::New(false));
	}

	Port port;
	port.m_behavior = Port::StringToPortBehavior(portBehavior);
	port.m_protocol = Port::StringToPortProtocol(portProtcol);
	port.m_portNumber = portNumber;

	if (scriptConfigurationKeys.size() != scriptConfigurationValues.size())
	{
		cout << "ERROR: Size of key array is not equal to size of value array in scriptConfiguration" << endl;
		return scope.Close(Boolean::New(false));
	}

	for (uint i = 0; i < scriptConfigurationKeys.size(); i++)
	{
		port.m_scriptConfiguration[scriptConfigurationKeys[i]] = scriptConfigurationValues[i];
	}

	if(port.m_behavior == PORT_SCRIPT || port.m_behavior == PORT_TARPIT_SCRIPT)
	{
		port.m_scriptName = portScriptName;
	}

	portSet->AddPort(port);

	return scope.Close(Boolean::New(true));
}

int HoneydProfileBinding::AddPortSet()
{
	m_profile->m_portSets.push_back(new PortSet());
	return m_profile->m_portSets.size() - 1;
}

Handle<Value> HoneydProfileBinding::SetPortSetBehavior(const Arguments& args)
{
	if( args.Length() != 3 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with five parameters")));
	}

	HandleScope scope;
	HoneydProfileBinding* obj = ObjectWrap::Unwrap<HoneydProfileBinding>(args.This());

	int portSetIndex = cvv8::CastFromJS<int>( args[0] );
	string protocol = cvv8::CastFromJS<string>( args[1] );
	string behavior = cvv8::CastFromJS<string>( args[2] );
	
	PortSet *portSet = obj->m_profile->GetPortSet(portSetIndex);
	if(portSet == NULL)
	{
		return scope.Close(Boolean::New(false));
	}


	if (protocol == "tcp")
	{
		portSet->m_defaultTCPBehavior = Port::StringToPortBehavior(behavior);
	} 
	else if (protocol == "udp")
	{
		portSet->m_defaultUDPBehavior = Port::StringToPortBehavior(behavior);
	} 
	else if (protocol == "icmp")
	{
		portSet->m_defaultICMPBehavior = Port::StringToPortBehavior(behavior);
	} 
	else 
	{
		return scope.Close(Boolean::New(false));
	}

	return scope.Close(Boolean::New(true));
}

bool HoneydProfileBinding::ClearPorts()
{
	//Delete the port sets, then clear the list of them
	for(uint i = 0; i < m_profile->m_portSets.size(); i++)
	{
		delete m_profile->m_portSets[i];
	}
	m_profile->m_portSets.clear();

	return true;
}

bool HoneydProfileBinding::SetIsPersonalityInherited(bool val) {
	m_profile->m_isPersonalityInherited = val;
	return true;
}

bool HoneydProfileBinding::SetIsDropRateInherited(bool val) {
	m_profile->m_isDropRateInherited = val;
	return true;
}

bool HoneydProfileBinding::SetIsUptimeInherited(bool val) {
	m_profile->m_isUptimeInherited = val;
	return true;
}

bool HoneydProfileBinding::Save() {
	bool ret = HoneydConfiguration::Inst()->AddProfile(m_profile);
	m_profile = NULL;
	return ret;
}

void HoneydProfileBinding::Init(v8::Handle<Object> target)
{
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("HoneydProfileBinding"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// Prototype
	Local<Template> proto = tpl->PrototypeTemplate();

	proto->Set("SetPersonality",	FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  std::string, &HoneydProfileBinding::SetPersonality>));
	proto->Set("SetCount",			FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  int,		&HoneydProfileBinding::SetCount>));
	proto->Set("AddPortSet",		FunctionTemplate::New(InvokeMethod<int, HoneydProfileBinding,  &HoneydProfileBinding::AddPortSet>));
	proto->Set("ClearPorts",		FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  &HoneydProfileBinding::ClearPorts>));
	proto->Set("SetIsPersonalityInherited",	FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  bool, &HoneydProfileBinding::SetIsPersonalityInherited>));
	proto->Set("SetIsDropRateInherited",	FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  bool, &HoneydProfileBinding::SetIsDropRateInherited>));
	proto->Set("SetIsUptimeInherited",		FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  bool, &HoneydProfileBinding::SetIsUptimeInherited>));
	proto->Set("Save",		FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding, &HoneydProfileBinding::Save>));
	
	proto->Set("ClearBroadcasts",		FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  &HoneydProfileBinding::ClearBroadcasts>));
	proto->Set("ClearProxies",		FunctionTemplate::New(InvokeMethod<bool, HoneydProfileBinding,  &HoneydProfileBinding::ClearProxies>));

	proto->Set("SetUptimeMin",		FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile, uint, &Profile::SetUptimeMin>));
	proto->Set("SetUptimeMax",		FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile,  uint,		&Profile::SetUptimeMax>));
	proto->Set("SetDropRate",		FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydProfileBinding, Profile,  std::string, &Profile::SetDropRate>));

	proto->Set("AddBroadcast",		FunctionTemplate::New(AddBroadcast)->GetFunction());
	proto->Set("AddProxy",		FunctionTemplate::New(AddProxy)->GetFunction());

	//Odd ball out, because it needs 5 parameters. More than InvoleWrappedMethod can handle
	proto->Set(String::NewSymbol("AddPort"),FunctionTemplate::New(AddPort)->GetFunction());
	proto->Set(String::NewSymbol("SetVendors"),FunctionTemplate::New(SetVendors)->GetFunction());
	proto->Set(String::NewSymbol("SetPortSetBehavior"),FunctionTemplate::New(SetPortSetBehavior)->GetFunction());

	Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
	target->Set(String::NewSymbol("HoneydProfileBinding"), constructor);
}


v8::Handle<Value> HoneydProfileBinding::New(const Arguments& args)
{
	if(args.Length() != 2)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with at exactly two parameters")));
	}

	std::string parentName = cvv8::CastFromJS<std::string>(args[0]);
	std::string profileName = cvv8::CastFromJS<std::string>(args[1]);

	HoneydProfileBinding* binding = new HoneydProfileBinding(parentName, profileName);
	binding->Wrap(args.This());
	return args.This();
}
