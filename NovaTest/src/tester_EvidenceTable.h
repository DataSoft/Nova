//============================================================================
// Name        : tester_EvidenceTable.h
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
// Description : This file contains unit tests for the class EvidenceTable
//============================================================================/*

#include "gtest/gtest.h"
#include "EvidenceTable.h"

using namespace Nova;

// The test fixture for testing class EvidenceTable.
class EvidenceTableTest : public ::testing::Test, public EvidenceTable
{

protected:

	EvidenceTable m_evidenceTable;
	Evidence *m_ev1, *m_ev2;

	EvidenceTableTest() {
		m_ev1 = NULL;
		m_ev2 = NULL;
	}

	bool InitEvidence()
	{
		if(m_ev1 != NULL)
		{
			delete m_ev1;
		}
		m_ev1 = new Evidence();
		if(m_ev2 != NULL)
		{
			delete m_ev1;
		}
		m_ev2 = new Evidence();
		return true;
	}
};

TEST_F(EvidenceTableTest, TestAll)
{
	InitEvidence();
	m_evidenceTable.InsertEvidence(m_ev1);
	Evidence *ev = m_evidenceTable.GetEvidence();
	EXPECT_EQ(ev, m_ev1);
	m_evidenceTable.InsertEvidence(m_ev2);
	m_evidenceTable.InsertEvidence(m_ev1);
	ev = m_evidenceTable.GetEvidence();
	EXPECT_EQ(ev, m_ev2);
	EXPECT_EQ(ev->m_next, m_ev1);
}

