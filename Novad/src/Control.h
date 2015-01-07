//============================================================================
// Name        : Control.h
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
// Description : Set of functions Novad can call to perform tasks related to
//			controlling the Novad process and operation
//============================================================================

#ifndef CONTROL_H_
#define CONTROL_H_

namespace Nova
{

//Saves the current classification state to file, and then exits the Novad process
//	param - used only to appease the signint handler. Unused.
void SaveAndExit(int param);

}

#endif /* CONTROL_H_ */
