//============================================================================
// Name        : GenericQueue.h
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
// Description : The GenericQueue object is a specialized linked-list
//			 of elementType objects for high performance IP packet processing
//============================================================================

#ifndef GENERICQUEUE_H_
#define GENERICQUEUE_H_

namespace Nova
{

template <class elementType>
class GenericQueue
{

public:
	GenericQueue()
	{
		m_first = NULL;
		m_last = NULL;
	}


	elementType *Pop()
	{
		elementType *ret = m_first;
		if(ret != NULL)
		{
			m_first = ret->m_next;
			//If this was the last item, set m_last to NULL, designates empty
			if(m_first == NULL)
			{
				m_last = NULL;
			}
			ret->m_next = NULL;
		}
		return ret;
	}

	elementType *PopAll()
	{
		elementType *ret = m_first;
		m_first = NULL;
		m_last = NULL;
		return ret;
	}

	//Returns true if this is the first piece of elementType
	bool Push(elementType *evidence)
	{
		//If m_last != NULL (There is evidence in the queue)
		if(m_last != NULL)
		{
			//Link the next node against the previous
			m_last->m_next = evidence;
			//m_last == last pushed evidence
			m_last = evidence;
			return false;
		}
		//If this is the first piece of evidence, set the front
		m_last = evidence;
		m_first = evidence;
		return true;
	}

private:

	elementType *m_first;
	elementType *m_last;
};

}

#endif /* GENERICQUEUE_H_ */
