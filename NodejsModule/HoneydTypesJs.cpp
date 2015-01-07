#include "HoneydTypesJs.h"
#include "v8Helper.h"
#include "Logger.h"

#include <v8.h>
#include <string>
#include <node.h>

using namespace v8;
using namespace Nova;
using namespace std;


Handle<Object> HoneydNodeJs::WrapBroadcast(Broadcast* bcast)
{
	HandleScope scope;

	if (broadcastTemplate.IsEmpty() )
	{
		Handle<FunctionTemplate> protoTemplate = FunctionTemplate::New();
		protoTemplate->InstanceTemplate()->SetInternalFieldCount(1);
		broadcastTemplate = Persistent<FunctionTemplate>::New(protoTemplate);
		
		Local<Template> proto = broadcastTemplate->PrototypeTemplate();
		proto->Set("GetScript",  FunctionTemplate::New(InvokeMethod<string, Broadcast, &Nova::Broadcast::GetScript>) );
		proto->Set("GetSrcPort",  FunctionTemplate::New(InvokeMethod<int, Broadcast, &Nova::Broadcast::GetSrcPort>) );
		proto->Set("GetDstPort",  FunctionTemplate::New(InvokeMethod<int, Broadcast, &Nova::Broadcast::GetDstPort>) );
		proto->Set("GetTime",  FunctionTemplate::New(InvokeMethod<int, Broadcast, &Nova::Broadcast::GetTime>) );
	}
	
	
	// Get the constructor from the template
	Handle<Function> ctor = broadcastTemplate->GetFunction();
	// Instantiate the object with the constructor
	Handle<Object> result = ctor->NewInstance();
	// Wrap the native object in an handle and set it in the internal field to get at later.
	Handle<External> broadcastPtr = External::New(bcast);
	result->SetInternalField(0,broadcastPtr);

	return scope.Close(result);
}


Handle<Object> HoneydNodeJs::WrapProxy(Proxy* proxy)
{
	HandleScope scope;

	if (proxyTemplate.IsEmpty() )
	{
		Handle<FunctionTemplate> protoTemplate = FunctionTemplate::New();
		protoTemplate->InstanceTemplate()->SetInternalFieldCount(1);
		proxyTemplate = Persistent<FunctionTemplate>::New(protoTemplate);
		
		Local<Template> proto = proxyTemplate->PrototypeTemplate();
		proto->Set("GetHoneypotPort",  FunctionTemplate::New(InvokeMethod<int, Proxy, &Nova::Proxy::GetHoneypotPort>) );
		proto->Set("GetProxyIP",  FunctionTemplate::New(InvokeMethod<string, Proxy, &Nova::Proxy::GetProxyIP>) );
		proto->Set("GetProxyPort",  FunctionTemplate::New(InvokeMethod<int, Proxy, &Nova::Proxy::GetProxyPort>) );
		proto->Set("GetProtocol",  FunctionTemplate::New(InvokeMethod<string, Proxy, &Nova::Proxy::GetProtocol>) );
	}
	
	
	// Get the constructor from the template
	Handle<Function> ctor = proxyTemplate->GetFunction();
	// Instantiate the object with the constructor
	Handle<Object> result = ctor->NewInstance();
	// Wrap the native object in an handle and set it in the internal field to get at later.
	Handle<External> proxyPtr = External::New(proxy);
	result->SetInternalField(0,proxyPtr);

	return scope.Close(result);
}

Handle<Object> HoneydNodeJs::WrapNode(Node* node)
{
	HandleScope scope;
	// Setup the template for the type if it hasn't been already
	if( nodeTemplate.IsEmpty() )
	{
		Handle<FunctionTemplate> protoTemplate = FunctionTemplate::New();
		protoTemplate->InstanceTemplate()->SetInternalFieldCount(1);
		nodeTemplate = Persistent<FunctionTemplate>::New(protoTemplate);

		// Javascript methods
		Local<Template> proto = nodeTemplate->PrototypeTemplate();
		proto->Set("GetInterface",  FunctionTemplate::New(InvokeMethod<string, Node, &Nova::Node::GetInterface>) );
		proto->Set("GetProfile",    FunctionTemplate::New(InvokeMethod<string, Node, &Nova::Node::GetProfile>) );
		proto->Set("GetPortSet",    FunctionTemplate::New(InvokeMethod<int, Node, &Nova::Node::GetPortSet>) );
		proto->Set("GetIP",         FunctionTemplate::New(InvokeMethod<string, Node, &Nova::Node::GetIP>) );
		proto->Set("GetMAC",        FunctionTemplate::New(InvokeMethod<string, Node, &Nova::Node::GetMAC>) );
		proto->Set("IsEnabled",     FunctionTemplate::New(InvokeMethod<bool, Node, &Nova::Node::IsEnabled>) );
	}

	// Get the constructor from the template
	Handle<Function> ctor = nodeTemplate->GetFunction();
	// Instantiate the object with the constructor
	Handle<Object> result = ctor->NewInstance();
	// Wrap the native object in an handle and set it in the internal field to get at later.
	Handle<External> nodePtr = External::New(node);
	result->SetInternalField(0,nodePtr);

	return scope.Close(result);
}

Handle<Object> HoneydNodeJs::WrapPortSet(PortSet *portSet)
{
	HandleScope scope;

	// Setup the template for the type if it hasn't been already
	if( portSetTemplate.IsEmpty() )
	{
		Handle<FunctionTemplate> protoTemplate = FunctionTemplate::New();
		protoTemplate->InstanceTemplate()->SetInternalFieldCount(1);
		portSetTemplate = Persistent<FunctionTemplate>::New(protoTemplate);

		// Javascript methods
		Local<Template> proto = portSetTemplate->PrototypeTemplate();
		proto->Set("GetTCPBehavior",	FunctionTemplate::New(InvokeMethod<std::string, Nova::PortSet, &Nova::PortSet::GetTCPBehavior>) );
		proto->Set("GetUDPBehavior",	FunctionTemplate::New(InvokeMethod<std::string, Nova::PortSet, &Nova::PortSet::GetUDPBehavior>) );
		proto->Set("GetICMPBehavior",	FunctionTemplate::New(InvokeMethod<std::string, Nova::PortSet, &Nova::PortSet::GetICMPBehavior>) );

		proto->Set(String::NewSymbol("GetPorts"),FunctionTemplate::New(GetPorts)->GetFunction());
	}

	// Get the constructor from the template
	Handle<Function> ctor = portSetTemplate->GetFunction();
	// Instantiate the object with the constructor
	Handle<Object> result = ctor->NewInstance();
	// Wrap the native object in an handle and set it in the internal field to get at later.
	Handle<External> portSetPtr = External::New(portSet);
	result->SetInternalField(0,portSetPtr);

	return scope.Close(result);
}

Handle<Object> HoneydNodeJs::WrapScript(Nova::Script *script)
{
	HandleScope scope;

	if (scriptTemplate.IsEmpty()) {
		Handle<FunctionTemplate> protoTemplate = FunctionTemplate::New();
        protoTemplate->InstanceTemplate()->SetInternalFieldCount(1);
        scriptTemplate = Persistent<FunctionTemplate>::New(protoTemplate);

        // Javascript methods
        Local<Template> proto = scriptTemplate->PrototypeTemplate();
        proto->Set("GetName",	FunctionTemplate::New(InvokeMethod<std::string, Nova::Script, &Nova::Script::GetName>) );
        proto->Set("GetService",	FunctionTemplate::New(InvokeMethod<std::string, Nova::Script, &Nova::Script::GetService>) );
        proto->Set("GetOsClass",	FunctionTemplate::New(InvokeMethod<std::string, Nova::Script, &Nova::Script::GetOsClass>) );
        proto->Set("GetPath",	FunctionTemplate::New(InvokeMethod<std::string, Nova::Script, &Nova::Script::GetPath>) );
        
		proto->Set("GetDefaultProtocol",	FunctionTemplate::New(InvokeMethod<std::string, Nova::Script, &Nova::Script::GetDefaultProtocol>) );
        proto->Set("GetDefaultPort",	FunctionTemplate::New(InvokeMethod<std::string, Nova::Script, &Nova::Script::GetDefaultPort>) );

        proto->Set("GetIsConfigurable",	FunctionTemplate::New(InvokeMethod<bool, Nova::Script, &Nova::Script::GetIsConfigurable>) );
        proto->Set("GetOptions",	FunctionTemplate::New(InvokeMethod<std::map<std::string, std::vector<std::string>> , Nova::Script, &Nova::Script::GetOptions>) );
        proto->Set("GetOptionDescriptions",	FunctionTemplate::New(InvokeMethod<std::map<std::string, std::string> , Nova::Script, &Nova::Script::GetOptionDescriptions>) );


    }

    // Get the constructor from the template
    Handle<Function> ctor = scriptTemplate->GetFunction();
    // Instantiate the object with the constructor
    Handle<Object> result = ctor->NewInstance();
    // Wrap the native object in an handle and set it in the internal field to get at later.
    Handle<External> scriptPtr = External::New(script);
    result->SetInternalField(0,scriptPtr);

    return scope.Close(result);
	
}


Handle<Object> HoneydNodeJs::WrapPort(Port *port)
{
    HandleScope scope;

    // Setup the template for the type if it hasn't been already
    if( portTemplate.IsEmpty() )
    {
        Handle<FunctionTemplate> protoTemplate = FunctionTemplate::New();
        protoTemplate->InstanceTemplate()->SetInternalFieldCount(1);
        portTemplate = Persistent<FunctionTemplate>::New(protoTemplate);

        // Javascript methods
        Local<Template> proto = portTemplate->PrototypeTemplate();
        proto->Set("GetPortNum",	FunctionTemplate::New(InvokeMethod<uint, Nova::Port, &Nova::Port::GetPortNum>) );
        proto->Set("GetProtocol",	FunctionTemplate::New(InvokeMethod<std::string, Nova::Port, &Nova::Port::GetProtocol>) );
        proto->Set("GetBehavior",	FunctionTemplate::New(InvokeMethod<std::string, Nova::Port, &Nova::Port::GetBehavior>) );
        proto->Set("GetScriptName",	FunctionTemplate::New(InvokeMethod<std::string, Nova::Port, &Nova::Port::GetScriptName>) );
        proto->Set("GetService",	FunctionTemplate::New(InvokeMethod<std::string, Nova::Port, &Nova::Port::GetService>) );
        proto->Set("GetScriptConfiguration",	FunctionTemplate::New(InvokeMethod<std::map<std::string,std::string>, Nova::Port, &Nova::Port::GetScriptConfiguration>) );
    }

    // Get the constructor from the template
    Handle<Function> ctor = portTemplate->GetFunction();
    // Instantiate the object with the constructor
    Handle<Object> result = ctor->NewInstance();
    // Wrap the native object in an handle and set it in the internal field to get at later.
    Handle<External> portPtr = External::New(port);
    result->SetInternalField(0,portPtr);

    return scope.Close(result);
}

Handle<Value> HoneydNodeJs::GetPorts(const Arguments& args)
{
	HandleScope scope;

	v8::Local<v8::Array> portArray = v8::Array::New();

	PortSet *portSet = ObjectWrap::Unwrap<PortSet>(args.This());
	if(portSet == NULL)
	{
		return scope.Close(portArray);
	}

	for(uint i = 0; i < portSet->m_portExceptions.size(); i++)
	{
		Port *copy = new Port();
		*copy = portSet->m_portExceptions[i];
		portArray->Set(v8::Number::New(i), HoneydNodeJs::WrapPort(copy));
	}

	return scope.Close(portArray);
}

Handle<Object> HoneydNodeJs::WrapProfile(Profile *pfile)
{
	HandleScope scope;
	// Setup the template for the type if it hasn't been already
	if( profileTemplate.IsEmpty() )
	{
		Handle<FunctionTemplate> protoTemplate = FunctionTemplate::New();
		protoTemplate->InstanceTemplate()->SetInternalFieldCount(1);
		profileTemplate = Persistent<FunctionTemplate>::New(protoTemplate);

		// Javascript methods
		Local<Template> proto = profileTemplate->PrototypeTemplate();
		proto->Set("GetName",			FunctionTemplate::New(InvokeMethod<std::string, Profile, &Nova::Profile::GetName>));
		proto->Set("GetPersonality",	FunctionTemplate::New(InvokeMethod<std::string, const Profile, &Nova::Profile::GetPersonality>));
		proto->Set("GetUptimeMin",		FunctionTemplate::New(InvokeMethod<uint, const Profile, &Nova::Profile::GetUptimeMin>));
		proto->Set("GetUptimeMax",		FunctionTemplate::New(InvokeMethod<uint, const Profile, &Nova::Profile::GetUptimeMax>));
		proto->Set("GetDropRate",		FunctionTemplate::New(InvokeMethod<std::string, const Profile, &Nova::Profile::GetDropRate>));
		proto->Set("GetCount",			FunctionTemplate::New(InvokeMethod<uint32_t, Profile, &Nova::Profile::GetCount>));
		proto->Set("GetParentProfile",	FunctionTemplate::New(InvokeMethod<std::string, const Profile, &Nova::Profile::GetParentProfile>));
		proto->Set("GetVendors",		FunctionTemplate::New(InvokeMethod<std::vector<std::string>, Profile, &Nova::Profile::GetVendors>));
		proto->Set("GetVendorCounts",	FunctionTemplate::New(InvokeMethod<std::vector<uint>, Profile, &Nova::Profile::GetVendorCounts>));

		proto->Set("IsPersonalityInherited",FunctionTemplate::New(InvokeMethod<bool, const Profile, &Nova::Profile::IsPersonalityInherited>));
		proto->Set("IsUptimeInherited",     FunctionTemplate::New(InvokeMethod<bool, const Profile, &Nova::Profile::IsUptimeInherited>));
		proto->Set("IsDropRateInherited",   FunctionTemplate::New(InvokeMethod<bool, const Profile, &Nova::Profile::IsDropRateInherited>));
	}

	// Get the constructor from the template
	Handle<Function> ctor = profileTemplate->GetFunction();
	// Instantiate the object with the constructor
	Handle<Object> result = ctor->NewInstance();
	// Wrap the native object in an handle and set it in the internal field to get at later.
	Handle<External> profilePtr = External::New(pfile);
	result->SetInternalField(0,profilePtr);

	return scope.Close(result);
}

Persistent<FunctionTemplate> HoneydNodeJs::nodeTemplate;
Persistent<FunctionTemplate> HoneydNodeJs::broadcastTemplate;
Persistent<FunctionTemplate> HoneydNodeJs::proxyTemplate;
Persistent<FunctionTemplate> HoneydNodeJs::portTemplate;
Persistent<FunctionTemplate> HoneydNodeJs::scriptTemplate;
Persistent<FunctionTemplate> HoneydNodeJs::portSetTemplate;
Persistent<FunctionTemplate> HoneydNodeJs::profileTemplate;
