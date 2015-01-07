//============================================================================
// Name        : Main.cpp
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
// Description : Wrapper function for the actual "main" to help in unit tests
//============================================================================

#include "Novad.h"
#include "Config.h"
#include "Logger.h"
#include <stdio.h>

using namespace Nova;

int main(int argc, char ** argv)
{
	if(argc > 1)
	{
		Config::Inst()->LoadCustomSettings(argc,argv);
	}

	return RunNovaD();
}
