#ifndef NOVACONFIGBINDING_H
#define NOVACONFIGBINDING_H

#include <node.h>
#include "Config.h"

class NovaConfigBinding : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> target);
	Nova::Config *GetChild();

private:
	NovaConfigBinding();
	~NovaConfigBinding();

	static v8::Handle<v8::Value> New(const v8::Arguments& args);

  static v8::Handle<v8::Value> ReadSetting(const v8::Arguments& args);
  static v8::Handle<v8::Value> WriteSetting(const v8::Arguments& args);
  static v8::Handle<v8::Value> ReloadConfiguration(const v8::Arguments& args);
  
  static v8::Handle<v8::Value> UseAllInterfaces(const v8::Arguments& args);
  static v8::Handle<v8::Value> UseAnyLoopback(const v8::Arguments& args);
  static v8::Handle<v8::Value> AddIface(const v8::Arguments& args);
  static v8::Handle<v8::Value> ClearInterfaces(const v8::Arguments& args);
  static v8::Handle<v8::Value> SetGroup(const v8::Arguments& args);
  static v8::Handle<v8::Value> SetDoppelInterface(const v8::Arguments& args);
  static v8::Handle<v8::Value> SetSMTPUseAuth(const v8::Arguments& args);
  
  Nova::Config *m_conf;
};

#endif
