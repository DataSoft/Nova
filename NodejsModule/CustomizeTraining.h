#ifndef CUSTOMIZETRAINING_H
#define CUSTOMIZETRAINING_H

#include <node.h>
#include <v8.h>
#include <fstream>
#include "v8Helper.h"
#include "TrainingData.h"
#include "Config.h"

class CustomizeTrainingBinding : public node::ObjectWrap
{
public:
	static void Init(v8::Handle<v8::Object> target);
	Nova::trainingSuspectMap * GetChild();
	static v8::Handle<v8::Value> ReturnAString(const v8::Arguments& args);
	static v8::Handle<v8::Value> GetDescriptions(const v8::Arguments& args);
	static v8::Handle<v8::Value> GetUIDs(const v8::Arguments& args);
	static v8::Handle<v8::Value> GetHostile(const v8::Arguments& args);
	static v8::Handle<v8::Value> SetIncluded(const v8::Arguments& args);
	static v8::Handle<v8::Value> Save(const v8::Arguments& args);
	static v8::Handle<v8::Value> GetCaptureIPs(const v8::Arguments& args);

private:
	CustomizeTrainingBinding();
	~CustomizeTrainingBinding();

	static v8::Handle<v8::Value> New(const v8::Arguments& args);

	Nova::trainingSuspectMap * m_map;
};

#endif
