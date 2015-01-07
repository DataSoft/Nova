//============================================================================
// Name        : WhitelistConfigurationBinding.h
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
// Description : Whitelist configuration wrapper class
//============================================================================

#ifndef WHITELISTCONFIGURATION_H
#define WHITELISTCONFIGURATION_H

#include <v8.h>
#include <node.h>

#include "WhitelistConfiguration.h"

using namespace node;
using namespace v8;
using namespace Nova;
using namespace std;

class WhitelistConfigurationBinding: ObjectWrap
{
private:

public:

	static void Init(Handle<Object> target);
	WhitelistConfigurationBinding();
	~WhitelistConfigurationBinding();

	static Handle<Value> New(const Arguments& args);
	static Persistent<FunctionTemplate> s_ct;

};


#endif
