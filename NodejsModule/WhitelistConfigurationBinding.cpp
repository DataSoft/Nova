//============================================================================
// Name        : WhitelistConfigurationBinding.cpp
// Copyright   : DataSoft Corporation 2011-2013
//      Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Exposes Nova_UI_Core as a module for the node.js environment.
//============================================================================

#include "WhitelistConfigurationBinding.h"
#include "v8Helper.h"

#include "Commands.h"
#include <vector>
#include <string>

using namespace std;

void WhitelistConfigurationBinding::Init(Handle<Object> target)
{
	HandleScope scope;

	Local<FunctionTemplate> t = FunctionTemplate::New(New);

	s_ct = Persistent<FunctionTemplate>::New(t);
	s_ct->InstanceTemplate()->SetInternalFieldCount(1);
	s_ct->SetClassName(String::NewSymbol("WhitelistConfigurationBinding"));

	// Javascript member methods
	//
	NODE_SET_PROTOTYPE_METHOD(s_ct, "AddEntry", (InvokeMethod<bool, string, Nova::WhitelistConfiguration::AddEntry>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "AddIp", (InvokeMethod<bool, string, string, Nova::WhitelistConfiguration::AddIp>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "AddIpRange", (InvokeMethod<bool, string, string, string, Nova::WhitelistConfiguration::AddIpRange>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "DeleteEntry", (InvokeMethod<bool, string, Nova::WhitelistConfiguration::DeleteEntry>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "GetIps", (InvokeMethod<vector<string>, Nova::WhitelistConfiguration::GetIps>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "GetIpRanges", (InvokeMethod<vector<string>, Nova::WhitelistConfiguration::GetIpRanges>) );

	// Javascript object constructor
	target->Set(String::NewSymbol("WhitelistConfigurationBinding"),
			s_ct->GetFunction());

}

WhitelistConfigurationBinding::WhitelistConfigurationBinding()
{
}

WhitelistConfigurationBinding::~WhitelistConfigurationBinding()
{

}

Handle<Value> WhitelistConfigurationBinding::New(const Arguments& args)
{
	HandleScope scope;
	WhitelistConfigurationBinding* hw = new WhitelistConfigurationBinding();
	hw->Wrap(args.This());
	return args.This();
}

Persistent<FunctionTemplate> WhitelistConfigurationBinding::s_ct;

