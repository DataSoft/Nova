#ifndef NOVAHASH_H_
#define NOVAHASH_H_

#include <exception>
#include <sys/types.h>
#include <unordered_map>

namespace Nova
{

// All hash map exceptions can be caught with this
class hashMapException : public std::exception {};

template <class KeyType, class ValueType, class HashFcn, class EqualKey>
class HashMap
{

public:

	HashMap();
	~HashMap();

	// Exception enabled access method
	ValueType& operator[](KeyType key);
	ValueType& get(KeyType key);

	bool keyExists(KeyType key);
	void erase(KeyType key);
	typename std::unordered_map<KeyType, ValueType, HashFcn, EqualKey>::iterator erase(typename std::unordered_map<KeyType, ValueType, HashFcn, EqualKey>::iterator key);

	// Expose generic methods we use
	void clear();
	uint size() const;
	bool empty() const;

	// Expose the iterators
	typedef typename std::unordered_map<KeyType, ValueType, HashFcn, EqualKey>::iterator iterator;

	typename std::unordered_map<KeyType, ValueType, HashFcn, EqualKey>::iterator begin();
	typename std::unordered_map<KeyType, ValueType, HashFcn, EqualKey>::iterator end();
	typename std::unordered_map<KeyType, ValueType, HashFcn, EqualKey>::iterator find(KeyType key);

private:

	std::unordered_map<KeyType, ValueType, HashFcn, EqualKey> m_map;

	EqualKey m_equalityChecker;
};

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
HashMap<KeyType,ValueType,HashFcn,EqualKey>::HashMap()
{
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
HashMap<KeyType,ValueType,HashFcn,EqualKey>::~HashMap()
{}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
typename std::unordered_map<KeyType, ValueType, HashFcn, EqualKey>::iterator HashMap<KeyType,ValueType,HashFcn,EqualKey>::begin()
{
	return m_map.begin();
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
typename std::unordered_map<KeyType, ValueType, HashFcn, EqualKey>::iterator HashMap<KeyType,ValueType,HashFcn,EqualKey>::end()
{
	return m_map.end();
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
typename std::unordered_map<KeyType, ValueType, HashFcn, EqualKey>::iterator HashMap<KeyType,ValueType,HashFcn,EqualKey>::erase(typename std::unordered_map<KeyType, ValueType, HashFcn, EqualKey>::iterator key)
{
	return m_map.erase(key);
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
typename std::unordered_map<KeyType, ValueType, HashFcn, EqualKey>::iterator HashMap<KeyType,ValueType,HashFcn,EqualKey>::find(KeyType key)
{
	return m_map.find(key);
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
void HashMap<KeyType,ValueType,HashFcn,EqualKey>::clear()
{
	m_map.clear();
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
void HashMap<KeyType,ValueType,HashFcn,EqualKey>::erase(KeyType key)
{
	m_map.erase(key);
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
uint HashMap<KeyType,ValueType,HashFcn,EqualKey>::size() const
{
	return m_map.size();
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
bool HashMap<KeyType,ValueType,HashFcn,EqualKey>::empty() const
{
	return m_map.empty();
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
bool HashMap<KeyType,ValueType,HashFcn,EqualKey>::keyExists(KeyType key)
{
	return m_map.find(key) != m_map.end();
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
ValueType& HashMap<KeyType,ValueType,HashFcn,EqualKey>::operator[](KeyType key)
{
	return m_map[key];
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
ValueType& HashMap<KeyType,ValueType,HashFcn,EqualKey>::get(KeyType key)
{
	return m_map[key];
}

}

#endif /* NOVAHASH_H_ */
