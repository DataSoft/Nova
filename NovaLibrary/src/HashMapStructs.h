//============================================================================
// Name        : HashMapStructs.h
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
// Description : Common hash map structure definitions for use with hashmap
//============================================================================

#ifndef HASHMAPSTRUCTS_H_
#define HASHMAPSTRUCTS_H_

#include "HashMap.h"

#include <arpa/inet.h>
#include <vector>
#include <string>

struct eqstr
{
  bool operator()(std::string k1, std::string k2) const
  {
    return !(k1.compare(k2));
  }
};

struct eqaddr
{
  bool operator()(in_addr_t k1, in_addr_t k2) const
  {
    return (k1 == k2);
  }
};

struct eqport
{
  bool operator()(in_port_t k1, in_port_t k2) const
  {
	    return (k1 == k2);
  }
};

struct eqint
{
  bool operator()(int k1, int k2) const
  {
	    return (k1 == k2);
  }
};

struct eqtime
{
  bool operator()(time_t k1, time_t k2) const
  {
	    return (k1 == k2);
  }
};

struct eqkey
{
	bool operator()(uint64_t k1, uint64_t k2) const
	{
		return (k1 == k2);
	}
};

struct eq_uint32_t
{
	bool operator()(uint32_t k1, uint32_t k2) const
	{
		return (k1 == k2);
	}
};

struct eq_uint16_t
{
	bool operator()(uint16_t k1, uint16_t k2) const
	{
		return (k1 == k2);
	}
};

#endif /* HASHMAPSTRUCTS_H_ */
