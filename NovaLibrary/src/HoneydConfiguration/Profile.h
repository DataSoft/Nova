//============================================================================
// Name        : Profile.h
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
// Description : Represents a single item in the PersonalityTree. A linked list
//		tree structure representing the hierarchy of profiles discovered
//============================================================================

#ifndef PROFILE_H_
#define PROFILE_H_

#include "Proxy.h"
#include "PortSet.h"
#include "Broadcast.h"

namespace Nova
{

class Profile
{

public:
	static const std::string EMPTY_PROFILE_NAME;


	Profile(Profile *parent, std::string key = "");
	Profile(std::string parentName, std::string key = "");

	~Profile();

	//Returns a string suitable for inserting into the honeyd configuration file
	std::string ToString(const int portSetIndex = 0, const std::string &nodeName = "default");

	//Returns a random vendor from the internal list of MAC vendors, according to the given probabilities
	// If a profile has no defined ethernet vendors, look to the parent profile
	//	returns an empty string "" on error
	std::string GetRandomVendor();

	//Returns a random PortSet from the internal list
	//	returns - -1 if no port sets present
	int GetRandomPortSet();

	//Returns the PortSet with the given name
	//	returns NULL if not found
	PortSet *GetPortSet(int portSetIndex);

	uint GetVendorCount(std::string vendorName);

	//Calculates the distributions of direct children of the given profile
	//	NOTE: Not recursive. Only calculates the given node's direct children.
	void RecalculateChildDistributions();

	//Copies the contents of the given profile into our own
	//	source - Profile to copy contents from
	//	returns - true on success, false on error
	//	NOTE: Works only on leaf profiles. Not interior profiles.
	bool Copy(Profile *source);

	//Recursively looks up what the personality of this profile should be according to
	//	Inheritance rules
	std::string GetPersonality() const;

	//Used when you really want the personality string of this profile, ignoring whether
	//	or not it is inherited
	std::string GetPersonalityNonRecursive();

	void SetPersonality(std::string personality);

	uint GetUptimeMin() const;

	uint GetUptimeMinNonRecursive() const;

	bool SetUptimeMin(uint uptime);

	uint GetUptimeMax() const;

	uint GetUptimeMaxNonRecursive() const;

	bool SetUptimeMax(uint uptime);

	std::string GetDropRate() const;

	std::string GetDropRateNonRecursive();

	bool SetDropRate(std::string droprate);

	//Javascript compatibility functions
	std::string GetName();
	uint32_t GetCount();
	std::string GetParentProfile() const;
	std::vector<std::string> GetVendors();
	std::vector<uint> GetVendorCounts();
	bool IsPersonalityInherited() const;
	bool IsUptimeInherited() const;
	bool IsDropRateInherited() const;
	//checks to see if the passed profile is equivalent to "this" profile
	bool IsEqual(const Profile &profile);
	bool IsEqualRecursive(const Profile &profile);

	// Number of hosts that have this personality
	uint32_t m_count;
	double m_distribution;

	// Name for the profile
	std::string m_name;

	bool m_isPersonalityInherited;
	bool m_isUptimeInherited;
	bool m_isDropRateInherited;

	// Vector of the child nodes to this node
	std::vector<Profile*> m_children;

	//Parent PersonalityTreeItem
	Profile *m_parent;

	// String representing the osclass. Used for matching ports to
	// scripts from the script table.
	std::string m_osclass;

	std::vector<std::pair<std::string, uint> > m_vendors;

	//A collection of PortSets, representing each group of ports found
	std::vector<PortSet *> m_portSets;

	std::vector<Broadcast *> m_broadcasts;
	std::vector<Proxy *> m_proxies;

private:

	// The average number of ports that this personality has
	uint16_t m_avgPortCount;

	//Variables affected by inheritance, thus these are private. Use their setters/getters to access them properly

	//The personality (usually Operating System) this profile will impersonate.
	//	This value is a direct line from the nmap-os-db
	std::string m_personality;

	std::string m_dropRate;

	//Upper and lower bound for set uptime
	uint m_uptimeMin;
	uint m_uptimeMax;

};

}

#endif /* PROFILE_H_ */
