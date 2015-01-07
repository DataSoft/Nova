//============================================================================
// Name        : tester_RequestMessage.h
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
// Description : This file contains unit tests for the class RequestMessage
//============================================================================/*

#include "gtest/gtest.h"

using namespace Nova;

// The test fixture for testing class RequestMessage.

class RequestMessageTestClass : Message_pb
{
	// So we can access protected members
};
class RequestMessageTest : public ::testing::Test {
protected:
	// Objects declared here can be used by all tests in the test case
	RequestMessageTestClass *testObject;

	// Unused methods here may be deleted
	RequestMessageTest() {
		// You can do set-up work for each test here.
	}


};
/* Disabled until updated for new RequestMessages
TEST_F(RequestMessageTest, test_constructorDestructor)
{
	RequestMessage *test1 = new RequestMessage(REQUEST_SUSPECTLIST);
	delete test1;
}

 *  Disabled until we can figure out how to test protected members
TEST_F(RequestMessageTest, test_RequestSuspectlist)
{
	testObject.m_requestType = REQUEST_SUSPECTLIST_REPLY;

	testObject.m_suspectList.push_back(42);
	testObject.m_suspectList.push_back(108);
	testObject.m_suspectList.push_back(17);

	uint32_t length;
	char *buffer;
	buffer = testObject.Serialize(&length);

	RequestMessage *copy = new RequestMessage(buffer, length);
	EXPECT_EQ(copy->m_suspectList.at(0), 42);
	EXPECT_EQ(copy->m_suspectList.at(1), 108);
	EXPECT_EQ(copy->m_suspectList.at(2), 17);
}
*/
