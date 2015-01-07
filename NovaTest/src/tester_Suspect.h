//============================================================================
// Name        : tester_Suspect.h
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
// Description : This file contains unit tests for the class Suspect
//============================================================================/*

#include "gtest/gtest.h"

#include "Suspect.h"
#include "EvidenceAccumulator.h"

using namespace Nova;
using namespace std;

#define LARGE_BUFFER_SIZE 65535

namespace {
// The test fixture for testing class Suspect.
class SuspectTest : public ::testing::Test {
protected:
	// Objects declared here can be used by all tests in the test case
	Suspect *suspect;
	Evidence p1, p2;


	SuspectTest() {
		suspect = new Suspect();

		// First test packet
		p1.m_evidencePacket.ip_p = 6;
		// These are just made up input values that make the math easy
		p1.m_evidencePacket.dst_port = 80;
		p1.m_evidencePacket.ip_dst = 1;
		p1.m_evidencePacket.ip_src = 123456;

		// Note: the byte order gets flipped for this
		p1.m_evidencePacket.ip_len = (uint16_t)256;
		p1.m_evidencePacket.ts = 10;
		p1.m_evidencePacket.tcp_hdr.syn = true;
		p1.m_evidencePacket.tcp_hdr.ack = false;


		// Second test packet
		p2.m_evidencePacket.ip_p = 6;
		p2.m_evidencePacket.dst_port = 20;
		p2.m_evidencePacket.ip_dst = 2;
		p2.m_evidencePacket.ip_src = 98765;

		// Note: the byte order gets flipped for this
		p2.m_evidencePacket.ip_len = (uint16_t)256;
		p2.m_evidencePacket.ts = 20;
		p2.m_evidencePacket.tcp_hdr.syn = true;
		p2.m_evidencePacket.tcp_hdr.ack = false;
	}

};

// Check adding and removing evidence
TEST_F(SuspectTest, EvidenceAddingRemoving)
{
	Evidence *t1 = new Evidence();
	Evidence *t2 = new Evidence();
	*t1 = p1;
	*t2 = p2;
	EXPECT_NO_FATAL_FAILURE(suspect->ReadEvidence(t1, true));
	EXPECT_NO_FATAL_FAILURE(suspect->ReadEvidence(t2, true));
}

TEST_F(SuspectTest, MacToString)
{
	suspect->m_lastMac = 81952921372024;
	EXPECT_EQ("78:45:C4:26:89:4A", suspect->GetMACString());
}

}

