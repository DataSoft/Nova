//============================================================================
// Name        : MessageManager.h
// Copyright   : DataSoft Corporation 2011-2013
//	Nova is free software: you can redistribute it and/or modify
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
// Description : Manages all incoming messages on sockets
//============================================================================

#ifndef MESSAGEMANAGER_H_
#define MESSAGEMANAGER_H_

#include <queue>
#include <map>
#include "pthread.h"
#include "event.h"
#include "protobuf/marshalled_classes.pb.h"

namespace Nova
{

// Filename of the IPC file Novad will listen on
#define NOVAD_LISTEN_FILENAME "/Novad_Listen"

class MessageManager
{

public:

	static MessageManager &Instance();

	//Grabs a message off of the message queue
	// Returns - A pointer to a valid Message object. Never NULL. Caller is responsible for life cycle of this message
	// NOTE: Blocking call. To be called from worker threads
	Message_pb *DequeueMessage();

	//Writes a given Message to the provided sessionIndex (0 for all sessions)
	//	message - A pointer to the message object to send
	//	sessionIndex - The index of the session to send the message to. (0 for all sessions)
	// Returns - true on successfully sending the object, false on error
	bool WriteMessage(Message_pb *message, uint32_t sessionIndex);

	//Writes a given Message to the all sessions except the given one
	//	message - A pointer to the message object to send
	//	sessionIndex - The index of the session to exclude
	// Returns - true on successfully sending the object, false on error
	bool WriteMessageExcept(Message_pb *message, uint32_t sessionIndex);

	//The following functions are only used internally (used from static functions, thus needing to be public)
	//Users of the messaging subsystem will not need to call these:

	//Puts a message onto the message queue
	void EnqueueMessage(Message_pb *message);

	//Function that gets called for every received packet
	//	Deserializes the bytes into a message, pushes that message into the right MessageQueue,
	//	and wakes up anyone waiting for the message.
	static void MessageDispatcher(struct bufferevent *bev, void *ctx);

	//Function that gets called for every socket meta-event, such as errors, shutdowns, and connections
	//	Contains logic to clean up after a dead socket, and setup new connections, or print errors
	static void ErrorDispatcher(struct bufferevent *bev, short error, void *ctx);
	static void DoAccept(evutil_socket_t listener, short event, void *arg);
	static void WriteDispatcher(struct bufferevent *bev, void *ctx);
	static void *AcceptDispatcher(void *);

	//Blocks until a WriteDispatch event has occurred with an empty buffer
	void WaitForFlush();

	//Return the next monotonically increasing session index number
	uint32_t GetNextSessionIndex();

	//When a socket closes, remove the session index from the current list
	void RemoveSessionIndex(uint32_t index);
	//When a new socket is created, add the session index to the list
	void AddSessionIndex(uint32_t index, struct bufferevent *bev);

	//Map of session indices to bufferevents
	pthread_mutex_t m_bevMapMutex;
	std::map<uint32_t, struct bufferevent*> m_bevMap;
private:

	static MessageManager *m_instance;

	//Constructor for MessageManager
	MessageManager();

	std::queue<Message_pb*> m_queue;
	pthread_mutex_t m_queueMutex;

	uint32_t m_sessionIndex;
	pthread_mutex_t m_sessionIndexMutex;

	pthread_cond_t m_popWakeupCondition;
};

}

#endif /* MESSAGEMANAGER_H_ */
