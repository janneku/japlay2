#ifndef _BENCODE_COMMON_H
#define _BENCODE_COMMON_H

#include <list>
#include <map>

void error(const char *fmt, ...);
void warning(const char *fmt, ...);

template<class T>
class list_iter {
public:
	list_iter(std::list<T> &list) :
		i(list.begin()),
		end(list.end())
	{
	}
	T operator *() const {
		return *i;
	}
	T *operator ->() const {
		return &*i;
	}
	bool valid() const
	{
		return i != end;
	}
	void next()
	{
		i++;
	}

private:
	typename std::list<T>::iterator i;
	typename std::list<T>::iterator end;
};

template<class T>
class list_rev_iter {
public:
	list_rev_iter(std::list<T> &list) :
		i(list.rbegin()),
		end(list.rend())
	{
	}
	T operator *() const {
		return *i;
	}
	T *operator ->() const {
		return &*i;
	}
	bool valid() const
	{
		return i != end;
	}
	void next()
	{
		i++;
	}

private:
	typename std::list<T>::reverse_iterator i;
	typename std::list<T>::reverse_iterator end;
};

template<class T>
class list_const_iter {
public:
	list_const_iter(const std::list<T> &list) :
		i(list.begin()),
		end(list.end())
	{
	}
	T operator *() const {
		return *i;
	}
	const T *operator ->() const {
		return &*i;
	}
	bool valid() const
	{
		return i != end;
	}
	void next()
	{
		i++;
	}

private:
	typename std::list<T>::const_iterator i;
	typename std::list<T>::const_iterator end;
};

template<class T>
class list_safe_iter {
public:
	list_safe_iter(std::list<T> &list) :
		i(list.begin()),
		next_i(list.begin()),
		end(list.end())
	{
		if (i != end)
			next_i++;
	}
	T operator *() const {
		return *i;
	}
	T *operator ->() const {
		return &*i;
	}
	bool valid() const
	{
		return i != end;
	}
	void next()
	{
		i = next_i;
		next_i++;
	}

private:
	typename std::list<T>::iterator i;
	typename std::list<T>::iterator next_i;
	typename std::list<T>::iterator end;
};

template<class Key, class Value>
class map: public std::map<Key, Value> {
public:
	map() {}
	map(const std::map<Key, Value> &from) :
		std::map<Key, Value>(from)
	{}

	void insert(const Key &key, const Value &value)
	{
		(*this)[key] = value;
	}

	Value get(const Key &key) const
	{
		typename std::map<Key, Value>::const_iterator i = find(key);
		if (i == std::map<Key, Value>::end())
			return Value();
		return i->second;
	}
};

template<class Key, class Value>
class map_iter {
public:
	map_iter(map<Key, Value> &list) :
		i(list.begin()),
		end(list.end())
	{
	}
	Key key() const
	{
		return i->first;
	}
	Value operator *() const {
		return i->second;
	}
	Value *operator ->() const {
		return &i->second;
	}
	bool valid() const
	{
		return i != end;
	}
	void next()
	{
		i++;
	}

private:
	typename map<Key, Value>::iterator i;
	typename map<Key, Value>::iterator end;
};

template<class Key, class Value>
class map_const_iter {
public:
	map_const_iter(const map<Key, Value> &list) :
		i(list.begin()),
		end(list.end())
	{
	}
	Key key() const
	{
		return i->first;
	}
	Value operator *() const {
		return i->second;
	}
	const Value *operator ->() const {
		return &i->second;
	}
	bool valid() const
	{
		return i != end;
	}
	void next()
	{
		i++;
	}

private:
	typename map<Key, Value>::const_iterator i;
	typename map<Key, Value>::const_iterator end;
};

template<class Key, class Value>
class map_safe_iter {
public:
	map_safe_iter(map<Key, Value> &list) :
		next_i(list.begin()),
		end(list.end())
	{
		next();
	}
	Key key() const
	{
		return i->first;
	}
	Value operator *() const {
		return i->second;
	}
	Value *operator ->() const {
		return &i->second;
	}
	bool valid() const
	{
		return i != end;
	}
	void next()
	{
		i = next_i;
		next_i++;
	}

private:
	typename map<Key, Value>::iterator i;
	typename map<Key, Value>::iterator next_i;
	typename map<Key, Value>::iterator end;
};

#endif
