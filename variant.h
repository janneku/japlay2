#ifndef _BENCODE_VARIANT_H
#define _BENCODE_VARIANT_H

#include "common.h"
#include <list>
#include <string>

class variant;
typedef std::list<variant> variant_list;
typedef map<std::string, variant> variant_map;

class variant
{
public:
	variant() :
		m_container(NULL)
	{}
	variant(const variant &from) :
		m_container(NULL)
	{
		if (from.m_container)
			m_container = from.m_container->clone();
	}
	variant(const variant_map &value) :
		m_container(new container<variant_map>(value))
	{
	}
	variant(const variant_list &value) :
		m_container(new container<variant_list>(value))
	{
	}

	template<class T>
	variant(const T &value) :
		m_container(new container<T>(value))
	{
	}

	template<class T>
	variant(const map<std::string, T> &value) :
		m_container(NULL)
	{
		variant_map dict;
		for (map_const_iter<std::string, T> i(value); i.valid();
		     i.next()) {
			dict.insert(i.key(), *i);
		}
		*this = dict;
	}

	template<class T>
	variant(const std::list<T> &value) :
		m_container(NULL)
	{
		variant_list list;
		for (list_const_iter<T> i(value); i.valid(); i.next()) {
			list.push_back(*i);
		}
		*this = list;
	}

	~variant()
	{
		delete m_container;
	}

	void operator =(const variant &from)
	{
		delete m_container;
		m_container = NULL;
		if (from.m_container)
			m_container = from.m_container->clone();
	}

	bool is_null() const
	{
		return m_container == NULL;
	}

	template<class T>
	int get_internal(T *value) const
	{
		container<T> *cont =
			dynamic_cast<container<T> *>(m_container);
		if (cont == NULL)
			return -1;
		*value = cont->data();
		return 0;
	}

	template<class T>
	int get(T *value) const
	{
		return get_internal(value);
	}

	template<class T>
	int get(std::list<T> *value) const
	{
		if (get_internal(value) == 0)
			return 0;
		/* try to convert */
		variant_list list;
		if (get_internal(&list))
			return -1;
		value->clear();
		for (list_const_iter<variant> i(list); i.valid(); i.next()) {
			T item;
			if (i->get(&item))
				return -1;
			value->push_back(item);
		}
		return 0;
	}

	template<class T>
	int get(map<std::string, T> *value) const
	{
		if (get_internal(value) == 0)
			return 0;
		/* try to convert */
		variant_map dict;
		if (get_internal(&dict))
			return -1;
		value->clear();
		for (map_const_iter<std::string, variant> i(dict); i.valid();
		     i.next()) {
			T item;
			if (i->get(&item))
				return -1;
			value->insert(i.key(), item);
		}
		return 0;
	}

private:
	class container_base
	{
	public:
		virtual container_base *clone() const = 0;
	};

	template<class T>
	class container: public container_base
	{
	public:
		container(const T &data) :
			m_data(data)
		{}
		void operator =(const T &data)
		{
			m_data = data;
		}
		T data() const { return m_data; }
		container_base *clone() const
		{
			return new container<T>(m_data);
		}
	private:
		T m_data;
	};

	container_base *m_container;
};

#endif
