//============================================================================
// Name        : ProfileTree.h
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
// Description : Contains a tree of profiles in their respective hierarchy,
//		along with useful helper functions
//============================================================================

#ifndef PERSONALITYTREE_H_
#define PERSONALITYTREE_H_

#include "Profile.h"
#include "ScannedHostTable.h"

namespace Nova
{

class ProfileTree
{

public:

	ProfileTree();

	~ProfileTree();

	// Returns a random (leaf) profile, according to the distributions given
	//	returns - A valid PersonalityNode on success, NULL on error
	Profile *GetRandomProfile();

	//Returns a pointer to the profile with the given name
	//	returns - NULL on error
	Profile *GetProfile(const std::string &name);

	// LoadTable first iterates over the persTable object and calls InsertHost on each
	// Personality* within the HashMap structure. It then calls GenerateProfiles recursively on
	// each subtree of the root node. Note that authentically this method is called within
	// the constructor, and thus persTable is passed down from the constructor's arguements;
	// if you want to generate a different tree structure from a different table, you need to
	// create a new instance of PersonalityTree.
	//  PersonalityTable *persTable - personalityTable to use for populating the ProfileTable
	//                           in the m_hdconfig object's m_profiles HashMap.
	// Returns nothing.
	bool LoadTable(ScannedHostTable *persTable);

	//Empty 'root' node of the tree, this node can be treated as the 'any' case or all personalities.
	Profile *m_root;

private:
	// InsertHost will create an item for the Personality,
	// add the item to that parent's children if not present (and aggregate the data of the
	// pre-existing node and the new data in *pers if it is present) and then continue to the
	// next PersonalityTreeItem. In this way, there exists one PersonalityTreeItem that represents the data
	// (in aggregate) of one or more ScannedHosts for each ScannedHosts in the ScannedHostsTable.
	//  ScannedHost *targetHost - ScannedHost object with which to create a new item or whose data
	//                      to add to a pre-existing node with the same ScannedHost.
	//	root - The root item of the PersonalityTree
	// Returns true on success, false on failure
	bool InsertHost(ScannedHost *targetHost, Profile *root, int persClassIndex);

};

}

#endif /* PERSONALITYTREE_H_ */
