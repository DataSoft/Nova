#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "VendorMacDbBinding.h"

using namespace std;
using namespace v8;

VendorMacDbBinding::VendorMacDbBinding()
{
	m_db = NULL;
}

VendorMacDbBinding::~VendorMacDbBinding()
{
	delete m_db;
}

VendorMacDb *VendorMacDbBinding::GetChild()
{
	return m_db;
}

void VendorMacDbBinding::Init(v8::Handle<Object> target)
{
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("VendorMacDbBinding"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// Prototype

	tpl->PrototypeTemplate()->Set(String::NewSymbol("GetVendorNames"),FunctionTemplate::New(InvokeWrappedMethod<vector<string>, VendorMacDbBinding, VendorMacDb, &VendorMacDb::GetVendorNames>));
	tpl->PrototypeTemplate()->Set(String::NewSymbol("GenerateRandomMAC"),FunctionTemplate::New(InvokeWrappedMethod<string, VendorMacDbBinding, VendorMacDb, string, &VendorMacDb::GenerateRandomMAC>));

	Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
	target->Set(String::NewSymbol("VendorMacDbBinding"), constructor);
}


v8::Handle<Value> VendorMacDbBinding::New(const Arguments& args)
{
	v8::HandleScope scope;

	VendorMacDbBinding *obj = new VendorMacDbBinding();
	obj->m_db = new VendorMacDb();
	obj->m_db->LoadPrefixFile();
	obj->Wrap(args.This());

	return args.This();
}

