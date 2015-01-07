//============================================================================
// Name        : tester_MessageSerilaization.h
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
// Description : This file contains unit tests for the class MessageSerilaization
//============================================================================/*

#include "gtest/gtest.h"

using namespace Nova;

// The test fixture for testing class MessageSerilaization.
class MessageSerilaizationTest : public ::testing::Test {
protected:
	uint32_t bufferSize;
	char *buffer;

	MessageSerilaizationTest() {}
};


TEST_F(MessageSerilaizationTest, REQUEST_SUSPECT)
{
	// Create and serialize a message
	Message_pb msg;
	msg.set_m_type(REQUEST_SUSPECT);
	SuspectID_pb *temp = msg.add_m_suspectids();
	temp->set_m_ip(42);
	temp->set_m_ifname("fake0");
	bufferSize = msg.ByteSize();
	buffer = new char[bufferSize];
	EXPECT_EQ(msg.SerializeToArray(buffer, bufferSize), true);

	// Deserialize it and make sure relevant internal variables for that message got copied okay
	Message_pb deserializedCopy;
	EXPECT_EQ(deserializedCopy.ParseFromArray(buffer, bufferSize), true);
	EXPECT_EQ(msg.m_suspectids(0).SerializeAsString(), deserializedCopy.m_suspectids(0).SerializeAsString());
	delete[] buffer;
}

TEST_F(MessageSerilaizationTest, REQEST_SUSPECT_REPLY)
{
	Message_pb msg;
	msg.set_m_type(REQUEST_SUSPECT_REPLY);
}

TEST_F(MessageSerilaizationTest, REQUEST_SUSPECT_LIST)
{
	Message_pb msg;
	msg.set_m_type(REQUEST_SUSPECTLIST_REPLY);

	SuspectID_pb *temp1 = msg.add_m_suspectids();
	temp1->set_m_ip(1);
	temp1->set_m_ifname("fake1");

	SuspectID_pb *temp2 = msg.add_m_suspectids();
	temp2->set_m_ip(2);
	temp2->set_m_ifname("fakeTestaoeuaoeuaoeu2");

	SuspectID_pb *temp3 = msg.add_m_suspectids();
	temp3->set_m_ip(3);
	temp3->set_m_ifname("f3");

	bufferSize = msg.ByteSize();
	buffer = new char[bufferSize];
	EXPECT_EQ(msg.SerializeToArray(buffer, bufferSize), true);

	Message_pb deserializedCopy;
	EXPECT_EQ(deserializedCopy.ParseFromArray(buffer, bufferSize), true);
	EXPECT_EQ(msg.m_suspectids(0).SerializeAsString(), deserializedCopy.m_suspectids(0).SerializeAsString());
	EXPECT_EQ(msg.m_suspectids(1).SerializeAsString(), deserializedCopy.m_suspectids(1).SerializeAsString());
	EXPECT_EQ(msg.m_suspectids(2).SerializeAsString(), deserializedCopy.m_suspectids(2).SerializeAsString());
	delete[] buffer;
}

TEST_F(MessageSerilaizationTest, REQUEST_SUSPECT_LIST_empty)
{
	Message_pb msg;
	msg.set_m_type(REQUEST_SUSPECTLIST_REPLY);
	bufferSize = msg.ByteSize();
	buffer = new char[bufferSize];
	EXPECT_EQ(msg.SerializeToArray(buffer, bufferSize), true);

	Message_pb deserializedCopy;
	EXPECT_EQ(deserializedCopy.ParseFromArray(buffer, bufferSize), true);
	EXPECT_EQ(msg.m_suspectids_size(), deserializedCopy.m_suspectids_size());
	delete[] buffer;
}

TEST_F(MessageSerilaizationTest, CONTROL_CLEAR_SUSPECT_REQUEST)
{
	// Create and serialize a message
	Message_pb msg;
	msg.set_m_type(CONTROL_CLEAR_SUSPECT_REQUEST);
	SuspectID_pb *temp = msg.mutable_m_suspectid();
	temp->set_m_ip(42);
	temp->set_m_ifname("fake0");

	bufferSize = msg.ByteSize();
	buffer = new char[bufferSize];
	EXPECT_EQ(msg.SerializeToArray(buffer, bufferSize), true);

	// Deserialize it and make sure relevant internal variables for that message got copied okay
	Message_pb deserializedCopy;
	EXPECT_EQ(deserializedCopy.ParseFromArray(buffer, bufferSize), true);
	EXPECT_EQ(msg.m_suspectid().SerializeAsString(), deserializedCopy.m_suspectid().SerializeAsString());
	delete[] buffer;
}


TEST_F(MessageSerilaizationTest, UPDATE_SUSPECT_CLEARED)
{
	// Create and serialize a message
	Message_pb msg;
	msg.set_m_type(UPDATE_SUSPECT_CLEARED);
	SuspectID_pb *temp = msg.mutable_m_suspectid();
	temp->set_m_ip(42);
	temp->set_m_ifname("fake0");

	bufferSize = msg.ByteSize();
	buffer = new char[bufferSize];
	EXPECT_EQ(msg.SerializeToArray(buffer, bufferSize), true);


	// Deserialize it and make sure relevant internal variables for that message got copied okay
	// The 4 offset is hacky, but it's to strip off the message size that would normally be removed in Message::Deserialize
	Message_pb deserializedCopy;
	EXPECT_EQ(deserializedCopy.ParseFromArray(buffer, bufferSize), true);
	EXPECT_EQ(msg.m_suspectid().SerializeAsString(), deserializedCopy.m_suspectid().SerializeAsString());
	delete[] buffer;
}


