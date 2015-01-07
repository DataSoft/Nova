#ifndef HONEYDPROFILEBINDING_H
#define HONEYDPROFILEBINDING_H

#include <node.h>
#include <v8.h>
#include "v8Helper.h"
#include "HoneydConfiguration/Profile.h"

class HoneydProfileBinding : public node::ObjectWrap
{

public:
	static void Init(v8::Handle<v8::Object> target);
  
	Nova::Profile *GetChild();

	bool SetPersonality(std::string personality);
	bool SetCount(int count);
	int AddPortSet();
	bool ClearPorts();
	bool ClearBroadcasts();
	bool ClearProxies();

	bool SetIsPersonalityInherited(bool);
	bool SetIsDropRateInherited(bool);
	bool SetIsUptimeInherited(bool);
	bool Save();

private:
	//The parent name is needed to know where to put the profile in the tree,
	//The profile name is the name of the profile to edit, or add
	HoneydProfileBinding(std::string parentName, std::string profileName);
	~HoneydProfileBinding();

	//Odd ball out, because it needs 5 parameters. More than InvoleWrappedMethod can handle
	static v8::Handle<v8::Value> AddPort(const v8::Arguments& args);
	static v8::Handle<v8::Value> SetVendors(const v8::Arguments& args);
	static v8::Handle<v8::Value> SetPortSetBehavior(const v8::Arguments& args);
	static v8::Handle<v8::Value> AddBroadcast(const v8::Arguments& args);
	static v8::Handle<v8::Value> AddProxy(const v8::Arguments& args);

	static v8::Handle<v8::Value> New(const v8::Arguments& args);

	Nova::Profile *m_profile;
	bool isNewProfile;
};

#endif
