//============================================================================
// Name        : Doppelganger.h
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
// Description : Set of functions used by Novad for masking local host information
//============================================================================

#ifndef DOPPELGANGER_H_
#define DOPPELGANGER_H_

#include "DatabaseQueue.h"
#include "protobuf/marshalled_classes.pb.h"

namespace Nova
{

class Doppelganger
{

public:

	// suspects: Uses the hostile suspects in this suspect table to determine Dopp routing
	Doppelganger(DatabaseQueue& suspects);
	~Doppelganger();

	//Synchronizes an initialized Doppelganger object with it's suspect table
	// *Note if the Dopp was never initialized this function initializes it.
	void UpdateDoppelganger();

	//Clears the routing rules, this disables the doppelganger until init is called again.
	void ClearDoppelganger();

	//Initializes the base routing rules the Doppelganger needs to operate.
	// Note: This function will simply return without executing if the Doppelganger has
	// called InitDoppelganger since construction or the last ClearDoppelganger();
	void InitDoppelganger();

	//Clears and Initializes the Doppelganger then updates the routing list from scratch.
	void ResetDoppelganger();

private:

	DatabaseQueue& m_suspectTable;
	std::vector<SuspectID_pb> m_suspectKeys;
	bool m_initialized;

};
}

#endif /* DOPPELGANGER_H_ */
