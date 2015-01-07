//============================================================================
// Name        : NovaNode.cpp
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

#include "NovaNode.h"
#include "Suspect.h"
#include "Config.h"
#include "HashMapStructs.h"
#include "MessageManager.h"

#include <map>

using namespace node;
using namespace v8;
using namespace Nova;
using namespace std;

map<int32_t, Persistent<Function>> jsCallbacks;
int32_t messageID = 0;

void NovaNode::InitNovaCallbackProcessing()
{
	uv_work_t *req = new uv_work_t;
	req->data = NULL;
	uv_queue_work(uv_default_loop(), req, NovaCallbackHandling, (uv_after_work_cb)AfterNovaCallbackHandling);
}

bool NovaNode::CheckInitNova()
{
	if(Nova::IsNovadConnected())
	{
		return true;
	}

	if(!Nova::ConnectToNovad())
	{
		LOG(WARNING, "Could not connect to Novad. It is likely down.","");
		return false;
	}

	return false;
}

void NovaNode::NovaCallbackHandling(uv_work_t*)
{
	LOG(DEBUG, "Initializing Novad callback processing","");

	while(true)
	{
		Nova::Message_pb *message = DequeueUIMessage();

		switch(message->m_type())
		{
			case REQUEST_SUSPECT_REPLY:
			{
				break;
			}
			case UPDATE_ALL_SUSPECTS_CLEARED:
			{
				HandleAllSuspectsCleared();
				break;
			}
			case UPDATE_SUSPECT_CLEARED:
			{
				Suspect *suspect = new Suspect();
				suspect->SetIdentifier(message->m_suspectid());
				LOG(DEBUG, "Got a clear suspect response for a suspect on interface " + message->m_suspectid().m_ifname(), "");
				HandleSuspectCleared(suspect);
				break;
			}
			case REQUEST_PONG:
			{
				break;
			}
			case CONNECTION_SHUTDOWN:
			{
				break;
			}
			default:
			{
				HandleCallbackError();
				break;
			}
		}
		delete message;
	}
}


void NovaNode::AfterNovaCallbackHandling(uv_work_t *req)
{
	delete req;
}

void NovaNode::HandleSuspectCleared(Suspect *suspect)
{
}

void NovaNode::HandleAllSuspectsCleared()
{
}


void NovaNode::HandleCallbackError()
{
	LOG(ERROR, "Novad provided CALLBACK_ERROR, will continue and move on","");
}

bool StopNovadWrapper()
{
	StopNovad();
	return true;
}

bool ReclassifyAllSuspectsWrapper()
{
	ReclassifyAllSuspects();
	return true;
}

bool DisconnectFromNovadWrapper()
{
	DisconnectFromNovad();
	return true;
}

void NovaNode::Init(Handle<Object> target)
{
	HandleScope scope;

	Local<FunctionTemplate> t = FunctionTemplate::New(New);

	s_ct = Persistent<FunctionTemplate>::New(t);
	s_ct->InstanceTemplate()->SetInternalFieldCount(1);
	s_ct->SetClassName(String::NewSymbol("NovaNode"));

	// Javascript member methods
	NODE_SET_PROTOTYPE_METHOD(s_ct, "GetFeatureNames", GetFeatureNames);
	NODE_SET_PROTOTYPE_METHOD(s_ct, "GetDIM", GetDIM);
	NODE_SET_PROTOTYPE_METHOD(s_ct, "GetSupportedEngines", GetSupportedEngines);

	NODE_SET_PROTOTYPE_METHOD(s_ct, "ClearAllSuspects", ClearAllSuspects);
	NODE_SET_PROTOTYPE_METHOD(s_ct, "CheckConnection", CheckConnection );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "CloseNovadConnection", (InvokeMethod<bool, DisconnectFromNovadWrapper>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "ConnectToNovad", (InvokeMethod<bool, Nova::ConnectToNovad>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "StartNovad", (InvokeMethod<bool, bool, Nova::StartNovad>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "StopNovad", (InvokeMethod<bool, StopNovadWrapper>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "HardStopNovad", (InvokeMethod<bool, Nova::HardStopNovad>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "StartHaystack", (InvokeMethod<bool, bool, Nova::StartHaystack>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "StopHaystack", (InvokeMethod<bool, Nova::StopHaystack>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "IsNovadConnected", (InvokeMethod<bool, Nova::IsNovadConnected>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "IsHaystackUp", (InvokeMethod<bool, Nova::IsHaystackUp>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "ReclassifyAllSuspects", (InvokeMethod<bool, ReclassifyAllSuspectsWrapper>) );
	
	NODE_SET_PROTOTYPE_METHOD(s_ct, "GetLocalIP", (InvokeMethod<std::string, std::string, Nova::GetLocalIP>) );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "GetSubnetFromInterface", (InvokeMethod<std::string, std::string, Nova::GetSubnetFromInterface>) );

	NODE_SET_PROTOTYPE_METHOD(s_ct, "Shutdown", Shutdown );
	NODE_SET_PROTOTYPE_METHOD(s_ct, "ClearSuspect", ClearSuspect );

	// Javascript object constructor
	target->Set(String::NewSymbol("Instance"), s_ct->GetFunction());

	InitMessaging();
	InitNovaCallbackProcessing();
}

// Figure out what the names of features are in the featureset
Handle<Value> NovaNode::GetFeatureNames(const Arguments &)
{
	HandleScope scope;

	vector<string> featureNames;
	for (int i = 0; i < DIM; i++)
	{
		featureNames.push_back(Nova::EvidenceAccumulator::m_featureNames[i]);
	}

	return scope.Close(cvv8::CastToJS(featureNames));
}

Handle<Value> NovaNode::ClearAllSuspects(const Arguments &)
{
	HandleScope scope;
	
	HandleAllSuspectsCleared();
	Nova::ClearAllSuspects();

	return scope.Close(Null());
}

Handle<Value> NovaNode::GetSupportedEngines(const Arguments &)
{
	HandleScope scope;

	return scope.Close(cvv8::CastToJS(Nova::Config::Inst()->GetSupportedEngines()));
}

Handle<Value> NovaNode::GetDIM(const Arguments &)
{
	HandleScope scope;

	return scope.Close(cvv8::CastToJS(DIM));
}

// Checks if we lost the connection. If so, tries to reconnect
Handle<Value> NovaNode::CheckConnection(const Arguments &)
{
	HandleScope scope;

	//LOG(DEBUG, "Attempting to connect to Novad...", "");

	Local<Boolean> result = Local<Boolean>::New( Boolean::New(CheckInitNova()) );
	return scope.Close(result);
}

Handle<Value> NovaNode::Shutdown(const Arguments &)
{
	HandleScope scope;
	LOG(DEBUG, "Shutdown... closing Novad connection","");

	Nova::DisconnectFromNovad();
	Local<Boolean> result = Local<Boolean>::New( Boolean::New(true) );
	return scope.Close(result);
}

Handle<Value> NovaNode::ClearSuspect(const Arguments &args)
{
	HandleScope scope;
	string suspectIp = cvv8::CastFromJS<string>(args[0]);
	string suspectInterface = cvv8::CastFromJS<string>(args[1]);

	in_addr_t address;
	inet_pton(AF_INET, suspectIp.c_str(), &address);

	SuspectID_pb id;
	id.set_m_ifname(suspectInterface);
	id.set_m_ip(ntohl(address));
	Nova::ClearSuspect(id);

	return scope.Close(Null());
}

NovaNode::NovaNode() :
			m_count(0)
{
}

NovaNode::~NovaNode()
{
	cout << "Destructing NovaNode." << endl;
}

Handle<Value> NovaNode::New(const Arguments& args)
{
	HandleScope scope;
	NovaNode* hw = new NovaNode();
	hw->Wrap(args.This());
	return args.This();
}

// Invoked when the only one referring to an OnNewSuspect handler is us, i.e. no JS objects
// are holding onto it.  So it's up to us to decide what to do about it.
void NovaNode::HandleOnNewSuspectWeakCollect(Persistent<Value> , void *)
{
	// For now, we do nothing, meaning that the callback will always stay registered
	// and continue to be invoked
	// even if the original object upon which OnNewSuspect() was invoked has been
	// let go.
}

Persistent<FunctionTemplate> NovaNode::s_ct;

pthread_t NovaNode::m_NovaCallbackThread=0;

