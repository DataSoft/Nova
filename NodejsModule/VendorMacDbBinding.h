#ifndef VENDORMACDBBINDING_H
#define VENDORMACDBBINDING_H

#include <node.h>
#include <v8.h>
#include "v8Helper.h"
#include "HoneydConfiguration/VendorMacDb.h"

class VendorMacDbBinding : public node::ObjectWrap {
public:
	static void Init(v8::Handle<v8::Object> target);
	VendorMacDb * GetChild();

private:
	VendorMacDbBinding();
	~VendorMacDbBinding();

	static v8::Handle<v8::Value> New(const v8::Arguments& args);

	VendorMacDb *m_db;
};

#endif
