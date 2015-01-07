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

#ifndef QUASARDATABASE_H_
#define QUASARDATABASE_H_

#include <string>
#include <sqlite3.h>
#include <stdexcept>

namespace Nova {

class QuasarDatabase {
public:
	static QuasarDatabase *Inst(std::string = "");

	int ResetPassword();
	virtual ~QuasarDatabase();

private:
	QuasarDatabase(std::string databaseFile);

	std::string m_databaseFile;
	static QuasarDatabase * m_instance;
	sqlite3 *db;


};
}

#endif /* QUASARDATABASE_H_ */
