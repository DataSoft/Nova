//============================================================================
// Name        : ClassificationEngineFactory.h
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
// Description :
//============================================================================

#include <string>
#include <vector>

#include "ClassificationEngine.h"

#ifndef CLASSIFICATIONENGINEFACTORY_H_
#define CLASSIFICATIONENGINEFACTORY_H_


// Factory method for classification engine creation. Creates instance of correct subclass based on config file settings.
// Returns NULL if the configuration file entry is invalid, otherwise a pointer to a new ClassificationEngine.
Nova::ClassificationEngine* MakeEngine(std::string engine);



#endif /* CLASSIFICATIONENGINEFACTORY_H_ */
