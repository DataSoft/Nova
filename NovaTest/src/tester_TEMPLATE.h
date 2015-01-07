/*
 * This an example unit test for a fake class YOURCLASS to show the general layout of things.
 * You can copy/paste this and use it as a template for making a new unit test.
 * Much of this is a style thing rather than hard and fast rules for how to set up a test.
 *

//============================================================================
// Name        : tester_YOURCLASS.h
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
// Description : This file contains unit tests for the class YOURCLASS
//============================================================================/*

#include "gtest/gtest.h"

#include "YOURCLASS.h"

using namespace Nova;

// The test fixture for testing class YOURCLASS.
class YOURCLASSTest : public ::testing::Test {
protected:
	// Objects declared here can be used by all tests in the test case
	YOURCLASS testObject;

	// Unused methods here may be deleted
	YOURCLASSTest() {
		// You can do set-up work for each test here.
	}
	virtual ~YOURCLASSTest() {
		// You can do clean-up work that doesn't throw exceptions here.
	}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:
	virtual void SetUp() {
		// Code here will be called immediately after the constructor (right before each test).
	}

	virtual void TearDown() {
		// Code here will be called immediately after each test (right before the destructor).
	}
};

// Tests go here. Multiple small tests are better than one large test, as each test
// will get a pass/fail and debugging information associated with it.

// Check that someMethod functions
TEST_F(YOURCLASSTest, test_someMethod)
{
	EXPECT_EQ(expectedValue, testObject.someMethod());
}

*/
