#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "TrainingDumpBinding.h"

using namespace std;
using namespace v8;
using namespace Nova;

TrainingDumpBinding::TrainingDumpBinding()
{
	m_db = NULL;
}

TrainingDumpBinding::~TrainingDumpBinding()
{
	delete m_db;
}

TrainingDump *TrainingDumpBinding::GetChild()
{
	return m_db;
}

void TrainingDumpBinding::Init(v8::Handle<Object> target)
{
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("TrainingDumpBinding"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// Prototype

	tpl->PrototypeTemplate()->Set(String::NewSymbol("LoadCaptureFile"),
		FunctionTemplate::New(InvokeWrappedMethod<bool, TrainingDumpBinding, TrainingDump, string, &TrainingDump::LoadCaptureFile>));
		
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SetIsHostile"),
		FunctionTemplate::New(InvokeWrappedMethod<bool, TrainingDumpBinding, TrainingDump, string, bool, &TrainingDump::SetIsHostile>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SetIsIncluded"),
		FunctionTemplate::New(InvokeWrappedMethod<bool, TrainingDumpBinding, TrainingDump, string, bool, &TrainingDump::SetIsIncluded>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SetDescription"),
		FunctionTemplate::New(InvokeWrappedMethod<bool, TrainingDumpBinding, TrainingDump, string, string, &TrainingDump::SetDescription>));

	tpl->PrototypeTemplate()->Set(String::NewSymbol("SetAllIsIncluded"),
		FunctionTemplate::New(InvokeWrappedMethod<bool, TrainingDumpBinding, TrainingDump, bool, &TrainingDump::SetAllIsIncluded>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SetAllIsHostile"),
		FunctionTemplate::New(InvokeWrappedMethod<bool, TrainingDumpBinding, TrainingDump, bool, &TrainingDump::SetAllIsHostile>));
	
	tpl->PrototypeTemplate()->Set(String::NewSymbol("SaveToDb"),
		FunctionTemplate::New(InvokeWrappedMethod<bool, TrainingDumpBinding, TrainingDump, string, &TrainingDump::SaveToDb>));


	Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
	target->Set(String::NewSymbol("TrainingDumpBinding"), constructor);
}


v8::Handle<Value> TrainingDumpBinding::New(const Arguments& args)
{
	v8::HandleScope scope;

	TrainingDumpBinding *obj = new TrainingDumpBinding();
	obj->m_db = new TrainingDump();
	obj->Wrap(args.This());

	return args.This();
}

