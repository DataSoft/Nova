#ifndef LOGGERBINDING_H
#define LOGGERBINDING_H

#include "Logger.h"
#include <node.h>

class LoggerBinding : public node::ObjectWrap {
	public:
	
	static void Init(v8::Handle<v8::Object> target);
	static v8::Handle<v8::Value> New(const v8::Arguments& args);
  	
	static v8::Handle<v8::Value> Log(const v8::Arguments& args);

	Nova::Logger *GetChild();
	Nova::Logger *m_logger;
};


#endif
