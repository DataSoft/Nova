//============================================================================
// Name        : NovaNode.h
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

#ifndef NOVANODE_H
#define NOVANODE_H

#include <v8.h>
#include <node.h>


/* Nova headers */
#include "nova_ui_core.h"
#include "NovaUtil.h"
#include "Suspect.h"
#include "Logger.h"

#include "v8Helper.h"

using namespace node;
using namespace v8;
using namespace Nova;
using namespace std;

class NovaNode: ObjectWrap
{
private:
	int m_count;
	static pthread_t m_NovaCallbackThread;
	static bool m_NovaCallbackHandlingContinue;

	static void InitNovaCallbackProcessing();
	static bool CheckInitNova();

	static void NovaCallbackHandling(uv_work_t*);
	static void AfterNovaCallbackHandling(uv_work_t*);
	static void HandleAllSuspectsCleared();
	static void HandleSuspectCleared(Suspect *);
	static void HandleCallbackError();

public:

	static Persistent<FunctionTemplate> s_ct;

	static void Init(Handle<Object> target);
	static Handle<Value> CheckConnection(const Arguments __attribute__((__unused__)) &args);
	static Handle<Value> Shutdown(const Arguments __attribute__((__unused__)) &args);
	static Handle<Value> ClearSuspect(const Arguments &args);
	NovaNode();
	~NovaNode();

	static Handle<Value> New(const Arguments& args);
	static Handle<Value> GetFeatureNames(const Arguments& args);
	static Handle<Value> GetDIM(const Arguments& args);
	static Handle<Value> GetSupportedEngines(const Arguments& args);
	static Handle<Value> ClearAllSuspects(const Arguments& args);
	static Handle<Value> registerOnAllSuspectsCleared(const Arguments& args);
	static Handle<Value> registerOnSuspectCleared(const Arguments& args);
	static void HandleOnNewSuspectWeakCollect(Persistent<Value> __attribute__((__unused__)) OnNewSuspectCallback, void __attribute__((__unused__)) * parameter);
};

#endif
