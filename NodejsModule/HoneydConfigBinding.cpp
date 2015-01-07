#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif
#include <node.h>
#include "HoneydConfigBinding.h"
#include "HoneydTypesJs.h"
#include "v8Helper.h"

using namespace v8;
using namespace Nova;
using namespace std;

HoneydConfigBinding::HoneydConfigBinding()
{
	m_conf = NULL;
};

HoneydConfigBinding::~HoneydConfigBinding()
{
	delete m_conf;
};

HoneydConfiguration *HoneydConfigBinding::GetChild()
{
	return m_conf;
}

void HoneydConfigBinding::Init(Handle<Object> target)
{
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("HoneydConfigBinding"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	tpl->PrototypeTemplate()->Set(String::NewSymbol("LoadAllTemplates"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::ReadAllTemplatesXML>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SaveAllTemplates"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::WriteAllTemplatesToXML>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("WriteHoneydConfiguration"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, &HoneydConfiguration::WriteHoneydConfiguration>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GenerateRandomUnusedMAC"),FunctionTemplate::New(InvokeWrappedMethod<string, HoneydConfigBinding, HoneydConfiguration, string, &HoneydConfiguration::GenerateRandomUnusedMAC>));

	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetProfileNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetProfileNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetLeafProfileNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetLeafProfileNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetNodeMACs"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetNodeMACs >));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetScriptNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetScriptNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetBroadcastScriptNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetBroadcastScriptNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetConfigurationsList"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, HoneydConfigBinding, HoneydConfiguration, &HoneydConfiguration::GetConfigurationsList>));

	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddScriptOptionValue"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, string, string, &HoneydConfiguration::AddScriptOptionValue >));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("DeleteScriptOptionValue"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, string, string, &HoneydConfiguration::DeleteScriptOptionValue >));

	tpl->PrototypeTemplate()->Set(String::NewSymbol("DeleteNode"),FunctionTemplate::New(InvokeWrappedMethod<bool, HoneydConfigBinding, HoneydConfiguration, string, &HoneydConfiguration::DeleteNode>));

	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddNodes"),FunctionTemplate::New(AddNodes)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetNode"),FunctionTemplate::New(GetNode)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("ChangeNodeInterfaces"),FunctionTemplate::New(ChangeNodeInterfaces)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddNode"),FunctionTemplate::New(AddNode)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetProfile"),FunctionTemplate::New(GetProfile)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetScript"),FunctionTemplate::New(GetScript)->GetFunction());


	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddScript"),FunctionTemplate::New(AddScript)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("RemoveScript"),FunctionTemplate::New(RemoveScript)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("DeleteScriptFromPorts"),FunctionTemplate::New(DeleteScriptFromPorts)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SaveAll"),FunctionTemplate::New(SaveAll)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("DeleteProfile"),FunctionTemplate::New(DeleteProfile)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("DeletePortSet"),FunctionTemplate::New(DeletePortSet)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddPortSet"),FunctionTemplate::New(AddPortSet)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPortSet"),FunctionTemplate::New(GetPortSet)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPortSetNames"),FunctionTemplate::New(GetPortSetNames)->GetFunction());
	
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetBroadcasts"),FunctionTemplate::New(GetBroadcasts)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetProxies"),FunctionTemplate::New(GetProxies)->GetFunction());

	tpl->PrototypeTemplate()->Set(String::NewSymbol("AddConfiguration"),FunctionTemplate::New(AddConfiguration)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("RemoveConfiguration"),FunctionTemplate::New(RemoveConfiguration)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SwitchConfiguration"),FunctionTemplate::New(SwitchConfiguration)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SetDoppelganger"),FunctionTemplate::New(SetDoppelganger)->GetFunction());

	Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
	target->Set(String::NewSymbol("HoneydConfigBinding"), constructor);
}


Handle<Value> HoneydConfigBinding::New(const Arguments& args)
{
	HandleScope scope;

	HoneydConfigBinding* obj = new HoneydConfigBinding();
	obj->m_conf = HoneydConfiguration::Inst();
	obj->Wrap(args.This());

	return args.This();
}

Handle<Value> HoneydConfigBinding::ChangeNodeInterfaces(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 2)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 2 parameters")));
	}

	std::vector<std::string> nodeNames = cvv8::CastFromJS<vector<string> >(args[0]);
	std::string newInterface = cvv8::CastFromJS<string>(args[1]);

	for(uint i = 0; i < nodeNames.size(); i++)
	{
		Node* node = obj->m_conf->GetNode(nodeNames[i]);
		node->m_interface = newInterface;
	}

	obj->m_conf->WriteAllTemplatesToXML();
	obj->m_conf->ReadAllTemplatesXML();

	return scope.Close(Boolean::New(true));
}

Handle<Value> HoneydConfigBinding::GetPortSet(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 2)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 2 parameters")));
	}

	std::string profileName = cvv8::CastFromJS<string>(args[0]);
	int portSetIndex = cvv8::CastFromJS<int>(args[1]);

	Profile *profile = obj->m_conf->GetProfile(profileName);
	if(profile == NULL)
	{
		return scope.Close( Null() );
	}
	PortSet *portSet = profile->GetPortSet(portSetIndex);
	if(portSet == NULL)
	{
		return scope.Close( Null() );
	}

	return scope.Close( HoneydNodeJs::WrapPortSet( portSet ));
}

Handle<Value> HoneydConfigBinding::GetPortSetNames(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 1 parameter")));
	}

	std::string profileName = cvv8::CastFromJS<string>(args[0]);

	Profile *profile = obj->m_conf->GetProfile(profileName);
	if(profile == NULL)
	{
		//ERROR
		return scope.Close( Null() );
	}

	v8::Local<v8::Array> portArray = v8::Array::New();
	for(uint i = 0; i < profile->m_portSets.size(); i++)
	{
		portArray->Set(v8::Number::New(i),cvv8::CastToJS(i));
	}

	return scope.Close( portArray );
}

Handle<Value> HoneydConfigBinding::GetBroadcasts(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if (args.Length() != 1) {
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 1 parameter")));
	}
	
	std::string profileName = cvv8::CastFromJS<string>(args[0]);
	
	Profile *profile = obj->m_conf->GetProfile(profileName);
	if(profile == NULL)
	{
		//ERROR
		return scope.Close( Null() );
	}

	v8::Local<v8::Array> bcastArray = v8::Array::New();
	for(uint i = 0; i < profile->m_broadcasts.size(); i++)
	{
		bcastArray->Set(v8::Number::New(i),HoneydNodeJs::WrapBroadcast(profile->m_broadcasts[i]));
	}

	return scope.Close( bcastArray );
}

Handle<Value> HoneydConfigBinding::GetProxies(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if (args.Length() != 1) {
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 1 parameter")));
	}
	
	std::string profileName = cvv8::CastFromJS<string>(args[0]);
	
	Profile *profile = obj->m_conf->GetProfile(profileName);
	if(profile == NULL)
	{
		//ERROR
		return scope.Close( Null() );
	}

	v8::Local<v8::Array> proxieArray = v8::Array::New();
	for(uint i = 0; i < profile->m_proxies.size(); i++)
	{
		proxieArray->Set(v8::Number::New(i),HoneydNodeJs::WrapProxy(profile->m_proxies[i]));
	}

	return scope.Close( proxieArray );
}

Handle<Value> HoneydConfigBinding::DeletePortSet(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 2)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 2 parameters")));
	}

	std::string profileToDeleteFrom = cvv8::CastFromJS<string>(args[0]);
	int portSetIndex = cvv8::CastFromJS<int>(args[1]);

	return scope.Close(Boolean::New(obj->m_conf->DeletePortSet(profileToDeleteFrom, portSetIndex)));
}

Handle<Value> HoneydConfigBinding::AddPortSet(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 1 parameter")));
	}

	std::string profileToDeleteFrom = cvv8::CastFromJS<string>(args[0]);

	return scope.Close(Boolean::New(obj->m_conf->AddPortSet(profileToDeleteFrom)));
}

Handle<Value> HoneydConfigBinding::DeleteProfile(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 1 parameter")));
	}

	std::string profileToDelete = cvv8::CastFromJS<string>(args[0]);

	return scope.Close(Boolean::New(obj->m_conf->DeleteProfile(profileToDelete)));
}

Handle<Value> HoneydConfigBinding::AddNodes(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if( args.Length() != 6 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 4 parameters")));
	}

	string profile = cvv8::CastFromJS<string>( args[0] );
	int portSetIndex = cvv8::CastFromJS<int>( args[1] );
	string vendor = cvv8::CastFromJS<string>( args[2] );
	string ipAddress = cvv8::CastFromJS<string>( args[3] );
	string interface = cvv8::CastFromJS<string>( args[4] );
	int count = cvv8::JSToInt32( args[5] );

	return scope.Close(Boolean::New(obj->m_conf->AddNodes(profile, portSetIndex, vendor, ipAddress, interface, count)));
}


Handle<Value> HoneydConfigBinding::AddNode(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if( args.Length() != 5 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 5 parameters")));
	}

	string profile = cvv8::CastFromJS<string>( args[0] );
	int portset = cvv8::CastFromJS<int>( args[1] );
	string ipAddress = cvv8::CastFromJS<string>( args[2] );
	string mac = cvv8::CastFromJS<string>( args[3] );
	string interface = cvv8::CastFromJS<string>( args[4] );

	return scope.Close(Boolean::New(obj->m_conf->AddNode(profile,ipAddress,mac, interface, portset)));
}


// xxx Bleh copy/paste code reuse from AddNode. Maybe give addNode an isDoppelganger bool or make it so we can make Node objects in JS
Handle<Value> HoneydConfigBinding::SetDoppelganger(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if( args.Length() != 5 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 5 parameters")));
	}

	Node node;
	node.m_pfile = cvv8::CastFromJS<string>( args[0] );
	node.m_portSetIndex = cvv8::CastFromJS<int>( args[1] );
	node.m_IP = cvv8::CastFromJS<string>( args[2] );
	node.m_MAC = cvv8::CastFromJS<string>( args[3] );
	node.m_interface = cvv8::CastFromJS<string>( args[4] );

	return scope.Close(Boolean::New(obj->m_conf->SetDoppelganger(node)));
}

Handle<Value> HoneydConfigBinding::GetNode(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if (args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}

	string MAC = cvv8::CastFromJS<string>(args[0]);

	Nova::Node *ret = new Node();

	// xxx horrible hack to reuse code by sticking "doppelganger" in the MAC field here
	if (MAC == "doppelganger")
	{
		*ret = obj->m_conf->GetDoppelganger();
	}
	else
	{
		ret = obj->m_conf->GetNode(MAC);
	}

	if (ret != NULL)
	{
		return scope.Close(HoneydNodeJs::WrapNode(ret));
	}
	else
	{
		return scope.Close( Null() );
	}
}

Handle<Value> HoneydConfigBinding::GetProfile(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if (args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}

	string name = cvv8::CastFromJS<string>(args[0]);
	Nova::Profile *ret = obj->m_conf->GetProfile(name);

	if (ret != NULL)
	{
		return scope.Close(HoneydNodeJs::WrapProfile(ret));
	}
	else
	{
		return scope.Close( Null() );
	}

}

Handle<Value> HoneydConfigBinding::GetScript(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if (args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}

	string name = cvv8::CastFromJS<string>(args[0]);
	Nova::Script *ret = new Nova::Script();
	*ret = obj->m_conf->GetScript(name);

	if (ret != NULL)
	{
		return scope.Close(HoneydNodeJs::WrapScript(ret));
	}
	else
	{
		return scope.Close( Null() );
	}

}

Handle<Value> HoneydConfigBinding::DeleteScriptFromPorts(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}

	string profileName = cvv8::CastFromJS<string>(args[0]);

	obj->m_conf->DeleteScriptFromPorts(profileName);

	return scope.Close(Boolean::New(true));
}

Handle<Value> HoneydConfigBinding::AddScript(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() > 4 || args.Length() < 2)
	{
		// The service should be found dynamically, but unfortunately
		// within the nmap services file the services are linked to ports,
		// whereas our scripts have no such discrete affiliation.
		// I'm thinking of adding a searchable dropdown (akin to the
		// dojo select within the edit profiles page) that contains
		// all the services, and allow the user to associate the
		// uploaded script with one of these services.
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 2 to 4 parameters (3rd parameter is optional service, 4th parameter is optional osclass")));
	}

	Nova::Script script;
	script.m_name = cvv8::CastFromJS<string>(args[0]);
	script.m_path = cvv8::CastFromJS<string>(args[1]);
	script.m_isConfigurable = false;

	if(args.Length() == 4)
	{
		script.m_service = cvv8::CastFromJS<string>(args[2]);
		script.m_osclass = cvv8::CastFromJS<string>(args[3]);
	}
	else
	{
		script.m_service = "";
		script.m_osclass = "";
	}

	if(obj->m_conf->AddScript(script))
	{
		return scope.Close(Boolean::New(true));
	}
	else
	{
		cout << "Script already present, doing nothing" << endl;
		return scope.Close(Boolean::New(false));
	}
}

Handle<Value> HoneydConfigBinding::RemoveScript(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 1 parameter")));
	}

	string scriptName = cvv8::CastFromJS<string>(args[0]);

	if(!obj->m_conf->DeleteScript(scriptName))
	{
		cout << "No registered script with name " << scriptName << endl;
		return scope.Close(Boolean::New(false));
	}

	return scope.Close(Boolean::New(true));
}

Handle<Value> HoneydConfigBinding::SaveAll(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	bool success = true;

	if (!obj->m_conf->WriteAllTemplatesToXML())
	{
		cout << "ERROR saving honeyd templates " << endl;
		success = false;
	}

	if (!obj->m_conf->WriteHoneydConfiguration()) {
		cout << "ERROR saving honeyd templates " << endl;
		success = false;
	}

	return scope.Close(Boolean::New(success));
}

Handle<Value> HoneydConfigBinding::AddConfiguration(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 3)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 3 parameters")));
	}

	string newConfigName = cvv8::CastFromJS<string>(args[0]);
	string clone = cvv8::CastFromJS<string>(args[1]);
	string cloneConfigName = cvv8::CastFromJS<string>(args[2]);
	bool cloneBool = false;

	if(!clone.compare("true"))
	{
		cloneBool = true;
	}

	return scope.Close(Boolean::New(obj->m_conf->AddNewConfiguration(newConfigName, cloneBool, cloneConfigName)));
}

Handle<Value> HoneydConfigBinding::RemoveConfiguration(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 1 parameter1")));
	}

	string config = cvv8::CastFromJS<string>(args[0]);

	if(!config.compare("default"))
	{
		cout << "Cannot remove default configuration" << endl;
		return scope.Close(Boolean::New(false));
	}

	return scope.Close(Boolean::New(obj->m_conf->RemoveConfiguration(config)));
}

Handle<Value> HoneydConfigBinding::SwitchConfiguration(const Arguments& args)
{
	HandleScope scope;
	HoneydConfigBinding* obj = ObjectWrap::Unwrap<HoneydConfigBinding>(args.This());

	if(args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with 1 parameter")));
	}

	string config = cvv8::CastFromJS<string>(args[0]);

	return scope.Close(Boolean::New(obj->m_conf->SwitchToConfiguration(config)));
}
