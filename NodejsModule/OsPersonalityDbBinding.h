#ifndef OSPERSONALITYDBBINDING_H
#define OSPERSONALITYDBBINDING_H

#include <node.h>
#include <v8.h>
#include "v8Helper.h"
#include "HoneydConfiguration/OsPersonalityDb.h"

class OsPersonalityDbBinding : public node::ObjectWrap {
public:
	static void Init(v8::Handle<v8::Object> target);

	OsPersonalityDb * GetChild();

private:
	OsPersonalityDbBinding();
	~OsPersonalityDbBinding();

	static v8::Handle<v8::Value> New(const v8::Arguments& args);

	OsPersonalityDb *m_db;
};

#endif
