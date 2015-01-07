#include "LoggerBinding.h"
#include "v8Helper.h"

using namespace std;
using namespace v8;
using namespace Nova;

void LoggerBinding::Init(Handle<Object> target) {
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("LoggerBinding"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
    
	
	tpl->PrototypeTemplate()->Set(String::NewSymbol("Log"),FunctionTemplate::New(Log)->GetFunction());

	Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
	target->Set(String::NewSymbol("LoggerBinding"), constructor);
}

Handle<Value> LoggerBinding::New(const Arguments& args) {
	HandleScope scope;

	LoggerBinding *obj = new LoggerBinding();
	obj->m_logger = Logger::Inst();
	obj->Wrap(args.This());
	
	return args.This();
}


Handle<Value> LoggerBinding::Log(const Arguments& args) {
	HandleScope scope;
  
  	LoggerBinding* obj = ObjectWrap::Unwrap<LoggerBinding>(args.This());
    
	string level = cvv8::CastFromJS<std::string>(args[0]);
	string basicString = cvv8::CastFromJS<std::string>(args[1]);
	string advancedString = cvv8::CastFromJS<std::string>(args[2]);
	string file = cvv8::CastFromJS<std::string>(args[3]);
	int line = cvv8::CastFromJS<int>(args[4]);

	// Default to emergency so invalid logger strings will get noticed
	Levels intLevel = EMERGENCY;
	if (level == "DEBUG") {
		intLevel = DEBUG;
	} else if (level == "INFO") {
		intLevel = INFO;
	} else if (level == "NOTICE") {
		intLevel = NOTICE;
	} else if (level == "WARNING") {
		intLevel = WARNING;
	} else if (level == "ERROR") {
		intLevel = ERROR;
	} else if (level == "CRITICAL") {
		intLevel = CRITICAL;
	} else if (level == "ALERT") {
		intLevel = ALERT;
	} else if (level == "EMERGENCY") {
		intLevel = EMERGENCY;
	} else {
		cout << "Invalid log level used in call to Log!" << endl;
	}

	obj->GetChild()->Log(intLevel, basicString.c_str(), advancedString.c_str(), file.c_str(), line);
	
	return scope.Close(Boolean::New(true));
}

Nova::Logger *LoggerBinding::GetChild() {
	return m_logger;
}
