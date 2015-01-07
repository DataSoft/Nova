//============================================================================
// Name        : Node.h
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
// Description : Represents a single haystack node
//============================================================================

#ifndef NODE_H_
#define NODE_H_

#include <boost/property_tree/ptree.hpp>

namespace Nova
{

class Node
{

public:

	std::string m_interface;
	std::string m_pfile;
	int m_portSetIndex;
	std::string m_IP;
	std::string m_MAC;
	bool m_enabled;


	Node() {}
	Node(const boost::property_tree::ptree &nodePtree)
	{
		m_interface = nodePtree.get<std::string>("interface");
		m_IP = nodePtree.get<std::string>("IP");
		m_enabled = nodePtree.get<bool>("enabled");
		m_MAC = nodePtree.get<std::string>("MAC");

		m_pfile = nodePtree.get<std::string>("profile.name");
		m_portSetIndex = nodePtree.get<int>("profile.portset");
	}

	boost::property_tree::ptree GetPtree()
	{
		boost::property_tree::ptree nodePtree;
		nodePtree.put<std::string>("interface", m_interface);
		nodePtree.put<std::string>("IP", m_IP);
		nodePtree.put<bool>("enabled", m_enabled);
		nodePtree.put<std::string>("MAC", m_MAC);

		nodePtree.put<std::string>("profile.name", m_pfile);
		nodePtree.put<int>("profile.portset", m_portSetIndex);

		return nodePtree;
	}

	// This is for the Javascript bindings in the web interface
	inline std::string GetInterface() {return m_interface;}
	inline std::string GetProfile() {return m_pfile;}
	inline int GetPortSet() {return m_portSetIndex;}
	inline std::string GetIP() {return m_IP;}
	inline std::string GetMAC() {return m_MAC;}
	inline bool IsEnabled() {return m_enabled;}
};

//Container for accessing node items
//Key - String representation of the MAC address used for the node
//value - The Node object itself
typedef Nova::HashMap<std::string, Node, std::hash<std::string>, eqstr > NodeTable;

}

#endif /* NODE_H_ */
