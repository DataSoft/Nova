#ifndef __HONEYDNODEJS_H
#define __HONEYDNODEJS_H

#include <v8.h>
#include "v8Helper.h"
#include "EvidenceAccumulator.h"
#include "HoneydConfiguration/Node.h"
#include "HoneydConfiguration/Profile.h"
#include "HoneydConfiguration/Port.h"
#include "HoneydConfiguration/Script.h"

class HoneydNodeJs : public node::ObjectWrap
{

public:
    static v8::Handle<v8::Object> WrapBroadcast(Nova::Broadcast* bcast);
    static v8::Handle<v8::Object> WrapProxy(Nova::Proxy* proxy);
    static v8::Handle<v8::Object> WrapNode(Nova::Node* node);
    static v8::Handle<v8::Object> WrapProfile(Nova::Profile *profile);
    static v8::Handle<v8::Object> WrapPort(Nova::Port *port);
    static v8::Handle<v8::Object> WrapScript(Nova::Script *script);
    static v8::Handle<v8::Object> WrapPortSet(Nova::PortSet *portSet);

    static v8::Handle<v8::Value> GetPorts(const v8::Arguments& args);

private:

	static v8::Persistent<v8::FunctionTemplate> nodeTemplate;
	static v8::Persistent<v8::FunctionTemplate> broadcastTemplate;
	static v8::Persistent<v8::FunctionTemplate> proxyTemplate;
	static v8::Persistent<v8::FunctionTemplate> profileTemplate;
	static v8::Persistent<v8::FunctionTemplate> portTemplate;
	static v8::Persistent<v8::FunctionTemplate> scriptTemplate;
	static v8::Persistent<v8::FunctionTemplate> portSetTemplate;
};


#endif // __SUSPECTJS_H
