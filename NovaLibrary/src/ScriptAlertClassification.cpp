//============================================================================
// Name        : ScriptAlertClassification.cpp
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
// Description : Child class of ClassificationEngine, specifies a threshold for
// classifying a device as an intruder
//============================================================================

#include <boost/algorithm/string.hpp>
#include <fstream>
#include <sstream>

#include "Database.h"
#include "Logger.h"
#include "Config.h"
#include "ScriptAlertClassification.h"

using namespace std;

namespace Nova
{

ScriptAlertClassification::ScriptAlertClassification()
{
	int res;

	string dbpath = Config::Inst()->GetPathHome() + "/data/scriptAlerts.db";

	SQL_RUN(SQLITE_OK, sqlite3_open(dbpath.c_str(), &db));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"SELECT * FROM script_alerts WHERE ip = ? AND interface = ?",
		-1, &getScriptAlerts,  NULL));

}

void ScriptAlertClassification::LoadConfiguration(string)
{
	// Could store the db path in a config file.
}

double ScriptAlertClassification::Classify(Suspect *suspect)
{
	int res;
	double classification = 0;

	suspect->m_classificationNotes += "\n=== Notes from Script Alert Classification Engine ===\n";

	SQL_RUN(SQLITE_OK, sqlite3_bind_text(getScriptAlerts, 1, suspect->GetIpString().c_str(), -1, SQLITE_TRANSIENT));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(getScriptAlerts, 2, suspect->GetInterface().c_str(), -1, SQLITE_TRANSIENT));


	res = sqlite3_step(getScriptAlerts);

	while(res == SQLITE_ROW)
	{
		suspect->m_classificationNotes += "Script '" + string((const char*)sqlite3_column_text(getScriptAlerts, 3)) +
				"' threw alert '" + string((const char*)sqlite3_column_text(getScriptAlerts, 4)) + "'\n";
		classification = 1;


		res = sqlite3_step(getScriptAlerts);
	}

	SQL_RUN(SQLITE_OK, sqlite3_reset(getScriptAlerts));

	return classification;
}

} /* namespace Nova */
