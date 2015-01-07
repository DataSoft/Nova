//============================================================================
// Name        : Point.h
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
// Description : Points paired with a classification for use in the Approximate
//					Nearest Neighbor Algorithm.
//============================================================================

#ifndef POINT_H_
#define POINT_H_

#include <ANN/ANN.h>
#include <sys/types.h>

namespace Nova
{

///Point class meant to encapsulate both an ANN Point along with a classification
class Point
{

public:

	///	The ANN Point, which represents this suspect in feature space
	ANNpoint m_annPoint;

	ANNpoint m_notNormalized;

	///	The classification given to the point on the basis of k-NN
	int m_classification;

	Point();
	Point(uint enabledFeatures);

	~Point();
};

}

#endif /* POINT_H_ */
