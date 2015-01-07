//============================================================================
// Name        : OsPersonalityDb.h
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
// Description : Object for using the nmap OS personality file
//============================================================================


#ifndef OSPERSONALITYDB_H_
#define OSPERSONALITYDB_H_

#include "../HashMapStructs.h"

#include <string>
#include <vector>


class OsPersonalityDb
{
public:
	OsPersonalityDb();

	void LoadNmapPersonalitiesFromFile();
	std::vector<std::string> GetPersonalityOptions();
	std::vector<std::pair<std::string, std::string> > m_nmapPersonalities;
};


#endif
