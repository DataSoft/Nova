#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif
#include <node.h>
#include "NovaConfigBinding.h"
#include "v8Helper.h"

using namespace v8;
using namespace Nova;
using namespace std;

NovaConfigBinding::NovaConfigBinding()
{
	m_conf = NULL;
};

NovaConfigBinding::~NovaConfigBinding()
{
	delete m_conf;
};

Config* NovaConfigBinding::GetChild()
{
	return m_conf;
}

void NovaConfigBinding::Init(Handle<Object> target) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("NovaConfigBinding"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("ListInterfaces"),FunctionTemplate::New(InvokeWrappedMethod<std::vector<std::string>, NovaConfigBinding, Config, &Config::ListInterfaces>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetGroup"),FunctionTemplate::New(InvokeWrappedMethod<std::string, NovaConfigBinding, Config, &Config::GetGroup>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("ListLoopbacks"),FunctionTemplate::New(InvokeWrappedMethod<std::vector<std::string>, NovaConfigBinding, Config, &Config::ListLoopbacks>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetInterfaces"),FunctionTemplate::New(InvokeWrappedMethod<std::vector<std::string>, NovaConfigBinding, Config, &Config::GetInterfaces>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetDoppelInterface"),FunctionTemplate::New(InvokeWrappedMethod<std::string, NovaConfigBinding, Config, &Config::GetDoppelInterface>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetUseAllInterfacesBinding"),FunctionTemplate::New(InvokeWrappedMethod<std::string, NovaConfigBinding, Config, &Config::GetUseAllInterfacesBinding>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("SetDoppelInterface"),FunctionTemplate::New(SetDoppelInterface)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("AddIface"),FunctionTemplate::New(AddIface)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("SetSMTPUseAuth"),FunctionTemplate::New(SetSMTPUseAuth)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("ClearInterfaces"),FunctionTemplate::New(ClearInterfaces)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("UseAllInterfaces"),FunctionTemplate::New(UseAllInterfaces)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetUseAnyLoopback"),FunctionTemplate::New(InvokeWrappedMethod<bool, NovaConfigBinding, Config, &Config::GetUseAnyLoopback>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("SetGroup"),FunctionTemplate::New(SetGroup)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("UseAnyLoopback"),FunctionTemplate::New(UseAnyLoopback)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("ReadSetting"),FunctionTemplate::New(ReadSetting)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("WriteSetting"),FunctionTemplate::New(WriteSetting)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("ReloadConfiguration"),FunctionTemplate::New(ReloadConfiguration)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetVersionString"),FunctionTemplate::New(InvokeWrappedMethod<string, NovaConfigBinding, Config, &Config::GetVersionString>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPathConfigHoneydHS"),FunctionTemplate::New(InvokeWrappedMethod<string, NovaConfigBinding, Config, &Config::GetPathConfigHoneydHS>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPathHome"),FunctionTemplate::New(InvokeWrappedMethod<string, NovaConfigBinding, Config, &Config::GetPathHome>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPathShared"),FunctionTemplate::New(InvokeWrappedMethod<string, NovaConfigBinding, Config, &Config::GetPathShared>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetCurrentConfig"),FunctionTemplate::New(InvokeWrappedMethod<string, NovaConfigBinding, Config, &Config::GetCurrentConfig>));

  tpl->PrototypeTemplate()->Set(String::NewSymbol("SetCurrentConfig"),FunctionTemplate::New(InvokeWrappedMethod<bool, NovaConfigBinding, Config, std::string, &Config::SetCurrentConfig>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetSMTPUseAuth"),FunctionTemplate::New(InvokeWrappedMethod<bool, NovaConfigBinding, Config, &Config::GetSMTPUseAuth>));
	
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetIpAddresses"),FunctionTemplate::New(InvokeMethod<std::vector<std::string>, std::string, Config::GetIpAddresses>));

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("NovaConfigBinding"), constructor);
}

Handle<Value> NovaConfigBinding::New(const Arguments& args)
{
	HandleScope scope;

	NovaConfigBinding* obj = new NovaConfigBinding();
	obj->m_conf = Config::Inst();
	obj->Wrap(args.This());

	return args.This();
}

Handle<Value> NovaConfigBinding::SetSMTPUseAuth(const Arguments& args)
{
  HandleScope scope;
  
  if(args.Length() != 1)
  {
    return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
  }
  
  NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
  std::string set = cvv8::CastFromJS<std::string>(args[0]);
  
  if(!set.compare("true"))
  {
    obj->m_conf->SetSMTPUseAuth(true);
  }
  else if(!set.compare("false"))
  {
    obj->m_conf->SetSMTPUseAuth(false);
  }
  else
  {
    cout << "Invalid parameter for SetSMTPUseAuth()" << endl;
  }
  
  return args.This();
}

Handle<Value> NovaConfigBinding::SetDoppelInterface(const Arguments& args)
{
	HandleScope scope;
	
	if(args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}
	
	NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
	std::string difToSet = cvv8::CastFromJS<std::string>(args[0]);
	
	obj->m_conf->SetDoppelInterface(difToSet);
	
	return args.This();
}

Handle<Value> NovaConfigBinding::SetGroup(const Arguments& args)
{
	HandleScope scope;
	
	if(args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}
	
	NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
	
	std::string selectedGroup = cvv8::CastFromJS<std::string>(args[0]);
	
	obj->m_conf->SetGroup(selectedGroup);
	
	return args.This();
}

Handle<Value> NovaConfigBinding::ClearInterfaces(const Arguments& args)
{
  HandleScope scope;
  NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());

  obj->m_conf->ClearInterfaces();

  return args.This();
}

Handle<Value> NovaConfigBinding::AddIface(const Arguments& args)
{
  HandleScope scope;
  NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
  
  if(args.Length() < 1)
  {
      return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
  }
  
  std::string pass = cvv8::CastFromJS<std::string>(args[0]);
  
  obj->m_conf->AddInterface(pass);
  
  return args.This();
}

Handle<Value> NovaConfigBinding::UseAllInterfaces(const Arguments& args) 
{
  HandleScope scope;
  NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());

  if( args.Length() < 1 )
  {
      return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
  }

  std::string def = cvv8::CastFromJS<std::string>( args[0] );

  if(!def.compare("true"))
  {
    obj->m_conf->SetUseAllInterfaces(true);
    obj->m_conf->SetInterfaces(obj->m_conf->ListInterfaces());
  }
  else
  {
    obj->m_conf->SetUseAllInterfaces(false);
  }
  

  return args.This();
}

Handle<Value> NovaConfigBinding::UseAnyLoopback(const Arguments& args) 
{
  HandleScope scope;
  NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());

    if( args.Length() < 1 )
    {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
    }

    bool def = cvv8::CastFromJS<bool>( args[0] );

  obj->m_conf->SetUseAnyLoopback(def);

  return args.This();
}

Handle<Value> NovaConfigBinding::ReadSetting(const Arguments& args) 
{
	HandleScope scope;
	NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());

	if( args.Length() < 1 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}

	string p1 = cvv8::CastFromJS<string>( args[0] );


	return scope.Close(String::New(obj->m_conf->ReadSetting(p1).c_str()));
}


Handle<Value> NovaConfigBinding::WriteSetting(const Arguments& args) 
{
	HandleScope scope;
	NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());

	if( args.Length() != 2 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with two parameters")));
	}

	string key = cvv8::CastFromJS<string>( args[0] );
	string value = cvv8::CastFromJS<string>( args[1] );


	return scope.Close(Boolean::New(obj->m_conf->WriteSetting(key, value)));
}

Handle<Value> NovaConfigBinding::ReloadConfiguration(const Arguments& args)
{
	HandleScope scope;
	NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());

	obj->m_conf->LoadConfig();

	return scope.Close(Boolean::New(true));
}
