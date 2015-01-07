//============================================================================
// Name        : tester_Database.h
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
// Description : This file contains unit tests for the class Database
//============================================================================/*

#include "gtest/gtest.h"

#include "ClassificationAggregator.h"
#include "Database.h"
#include "DatabaseQueue.h"

using namespace Nova;

extern ClassificationEngine *engine;


// The test fixture for testing class Database.
class DatabaseTest : public ::testing::Test {
protected:
	// Unused methods here may be deleted
	DatabaseTest() {
		// You can do set-up work for each test here.
		Database::Inst()->Connect();
	}
	virtual ~DatabaseTest() {
		// You can do clean-up work that doesn't throw exceptions here.
	}
};

class dummyce : public ClassificationEngine {
public:
	 double Classify(Nova::Suspect*)
	 {
		 return 0.5;
	 }
};

/*

TEST_F(DatabaseTest_DISABLED, testDatabase)
{
	engine = new dummyce();
	DatabaseQueue q;
	Evidence f;

	f.m_evidencePacket.ip_p = 17;
	f.m_evidencePacket.interface = "eth0";
	f.m_evidencePacket.ip_dst = 42;

	for (int i = 0; i < 250000; i++) {
		f.m_evidencePacket.dst_port = i;
		f.m_evidencePacket.ip_src = i;

		q.ProcessEvidence(&f, true);
	}

	struct timeval start, end;
	long mtime, seconds, useconds;
	gettimeofday(&start, NULL);

	q.WriteToDatabase();

	gettimeofday(&end, NULL);
	seconds  = end.tv_sec  - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;
	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
	cout << "Elapsed time in milliseconds: " << mtime << endl;

}

*/
