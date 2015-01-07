//============================================================================
// Name        : QuasarDatabase.h
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
// Description : Wrapper for adding entries to the Quasar SQL database
//============================================================================/*

#include "Logger.h"
#include "Database.h"
#include "QuasarDatabase.h"

#include <sstream>
#include <string>

using namespace std;

namespace Nova {

QuasarDatabase *QuasarDatabase::m_instance = NULL;

QuasarDatabase *QuasarDatabase::Inst(std::string databaseFile)
{
	if(m_instance == NULL)
	{
		m_instance = new QuasarDatabase(databaseFile);
	}
	return m_instance;
}

int QuasarDatabase::ResetPassword()
{
	if (!db) {
		return -1;
	}

	stringstream ss;
	ss << "REPLACE INTO credentials VALUES (\"nova\", \"934c96e6b77e5b52c121c2a9d9fa7de3fbf9678d\", \"root\")";

	char *zErrMsg = 0;
	int state = sqlite3_exec(db, ss.str().c_str(), NULL, 0, &zErrMsg);
	if (state != SQLITE_OK)
	{
		string errorMessage(zErrMsg);
		sqlite3_free(zErrMsg);
		return -1;
	}

	return 0;
}

QuasarDatabase::QuasarDatabase(std::string databaseFile) {
	if (databaseFile == "")
	{
		databaseFile = Config::Inst()->GetPathHome() + "/data/quasarDatabase.db";
	}
	m_databaseFile = databaseFile;

	int res;
	SQL_RUN(SQLITE_OK, sqlite3_open(m_databaseFile.c_str(), &db));

}

QuasarDatabase::~QuasarDatabase() {
	if (sqlite3_close(db) != SQLITE_OK)
	{
		LOG(ERROR, "Unable to close and finalize Quasar SQL database: " + string(sqlite3_errmsg(db)), "");
	}
}

}
