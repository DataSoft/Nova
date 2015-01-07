//============================================================================
// Name        : Lock.h
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
// Description : Simple wrapper class for easy use of pthread mutex locking
//============================================================================

#ifndef LOCK_H_
#define LOCK_H_

#include "pthread.h"

namespace Nova
{

enum lockType
{
	WRITE_LOCK,
	READ_LOCK
};

class Lock
{

public:
	Lock(pthread_mutex_t *lock)
	{
		m_isMutex = true;
		m_lockAquired = false;
		m_mutex = lock;

		if(!pthread_mutex_lock(m_mutex))
		{
			m_lockAquired = true;
		}
	}

	Lock(pthread_rwlock_t *lock, lockType type)
	{
		m_isMutex = false;
		m_lockAquired = false;
		m_rwlock = lock;


		if(type == READ_LOCK)
		{
			if(!pthread_rwlock_rdlock(m_rwlock))
			{
				m_lockAquired = true;
			}
		}
		else if(type == WRITE_LOCK)
		{
			if(!pthread_rwlock_wrlock(m_rwlock))
			{
				m_lockAquired = true;
			}
		}
	}

	Lock()
	{
		m_isMutex = true;
		m_lockAquired = false;
		m_mutex = NULL;
		m_rwlock = NULL;
	}

	~Lock()
	{
		// Only try to unlock if we acquired the lock okay
		if(m_lockAquired)
		{
			if(m_isMutex)
			{
				pthread_mutex_unlock(m_mutex);
			}
			else
			{
				pthread_rwlock_unlock(m_rwlock);
			}
		}
	}

private:
	bool m_isMutex;
	bool m_lockAquired;

	pthread_mutex_t *m_mutex;
	pthread_rwlock_t *m_rwlock;
};

}

#endif /* LOCK_H_ */
