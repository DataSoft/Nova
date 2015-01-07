#ifndef HONEYDCONFIGBINDING_H
#define HONEYDCONFIGBINDING_H

#include <node.h>
#include <v8.h>
#include "v8Helper.h"
#include "HoneydConfiguration/HoneydConfiguration.h"


class HoneydConfigBinding : public node::ObjectWrap
{

public:
	static void Init(v8::Handle<v8::Object> target);
	Nova::HoneydConfiguration* GetChild();

private:
	HoneydConfigBinding();
	~HoneydConfigBinding();
	Nova::HoneydConfiguration *m_conf;


	static v8::Handle<v8::Value> New(const v8::Arguments& args);

	static v8::Handle<v8::Value> AddNodes(const v8::Arguments& args);
	static v8::Handle<v8::Value> AddNode(const v8::Arguments& args);
	static v8::Handle<v8::Value> AddScript(const v8::Arguments& args);
	static v8::Handle<v8::Value> AddConfiguration(const v8::Arguments& args);
	static v8::Handle<v8::Value> RemoveConfiguration(const v8::Arguments& args);
	static v8::Handle<v8::Value> SwitchConfiguration(const v8::Arguments& args);
	static v8::Handle<v8::Value> RemoveScript(const v8::Arguments& args);
	static v8::Handle<v8::Value> ChangeNodeInterfaces(const v8::Arguments& args);

	static v8::Handle<v8::Value> GetNode(const v8::Arguments& args);
	static v8::Handle<v8::Value> GetProfile(const v8::Arguments& args);
	static v8::Handle<v8::Value> GetScript(const v8::Arguments& args);
	
	static v8::Handle<v8::Value> GetBroadcasts(const v8::Arguments& args);
	static v8::Handle<v8::Value> GetProxies(const v8::Arguments& args);

	static v8::Handle<v8::Value> GetPortSet(const v8::Arguments& args);
	static v8::Handle<v8::Value> GetPortSetNames(const v8::Arguments& args);

	static v8::Handle<v8::Value> DeleteScriptFromPorts(const v8::Arguments& args);

	static v8::Handle<v8::Value> SaveAll(const v8::Arguments& args);
	static v8::Handle<v8::Value> DeleteProfile(const v8::Arguments& args);
	static v8::Handle<v8::Value> DeletePortSet(const v8::Arguments& args);
	static v8::Handle<v8::Value> AddPortSet(const v8::Arguments& args);

	static v8::Handle<v8::Value> SetDoppelganger(const v8::Arguments& args);

};

#endif
