//============================================================================
// Name        : Subnet.h
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
// Description : Represents a single subnet that honeyd can be expected to operate on
//============================================================================
#ifndef SUBNET_H_
#define SUBNET_H_

namespace Nova
{

class Subnet
{

public:

	std::string m_name;
	std::string m_address;
	std::string m_mask;
	int m_maskBits;
	in_addr_t m_base;
	in_addr_t m_max;
	bool m_enabled;
	bool m_isRealDevice;
	std::vector<std::string> m_nodes;
	boost::property_tree::ptree m_tree;
};

}


#endif /* SUBNET_H_ */
