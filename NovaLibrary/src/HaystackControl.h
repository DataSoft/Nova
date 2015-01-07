//============================================================================
// Name        : HaystackControl.h
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
// Description : Controls the Honeyd Haystack and Doppelganger processes
//============================================================================

#include "Config.h"

#include <sstream>

namespace Nova
{
//Starts the Honeyd Haystack process
//	returns - True if haystack successfully started, false on error
//	NOTE: If the haystack is already running, this function does nothing and returns true
bool StartHaystack(bool blocking = false);

//Stops the Honeyd Haystack process
//	returns - True if haystack successfully stopped, false on error
//	NOTEL if the haystack is already dead, this function does nothing and returns true
bool StopHaystack();

//Returns whether the Haystack is running or not
//	returns - True if honeyd haystack is running, false if not running
bool IsHaystackUp();
}
