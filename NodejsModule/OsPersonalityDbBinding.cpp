#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "OsPersonalityDbBinding.h"

using namespace std;
using namespace v8;

OsPersonalityDbBinding::OsPersonalityDbBinding()
{
	m_db = NULL;
}

OsPersonalityDbBinding::~OsPersonalityDbBinding()
{
	delete m_db;
}

OsPersonalityDb *OsPersonalityDbBinding::GetChild()
{
	return m_db;
}

void OsPersonalityDbBinding::Init(v8::Handle<Object> target)
{
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("OsPersonalityDbBinding"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// Prototype

	//tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPersonalityOptions"),FunctionTemplate::New(GetPersonalityOptions)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetPersonalityOptions"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, OsPersonalityDbBinding, OsPersonalityDb, &OsPersonalityDb::GetPersonalityOptions>));

	Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
	target->Set(String::NewSymbol("OsPersonalityDbBinding"), constructor);
}


v8::Handle<Value> OsPersonalityDbBinding::New(const Arguments& args)
{
	v8::HandleScope scope;

	OsPersonalityDbBinding* obj = new OsPersonalityDbBinding();
	obj->m_db = new OsPersonalityDb();
	obj->m_db->LoadNmapPersonalitiesFromFile();
	obj->Wrap(args.This());

	return args.This();
}



