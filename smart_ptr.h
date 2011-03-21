#ifndef _SMART_PTR_H
#define _SMART_PTR_H

#include <string.h>
#include <assert.h>

template<class T>
class ptr {
public:
	ptr() :
		m_data(NULL), m_refcount(NULL)
	{}

	ptr(const ptr<T> &from) :
		m_data(from.m_data), m_refcount(from.m_refcount)
	{
		incref();
	}

	explicit ptr(T *data) :
		m_data(data)
	{
		assert(data != NULL);
		m_refcount = new unsigned int(1);
	}

	~ptr()
	{
		decref();
	}

	T *data() const { return m_data; }

	bool is_null() const { return m_refcount == NULL; }

	void operator =(const ptr<T> &from)
	{
		decref();
		m_data = from.m_data;
		m_refcount = from.m_refcount;
		incref();
	}

	T operator *() const {
		return *m_data;
	}
	T *operator ->() const {
		return m_data;
	}

private:
	T *m_data;
	unsigned int *m_refcount;

	void incref()
	{
		if (m_refcount == NULL)
			return;
		(*m_refcount)++;
	}
	void decref()
	{
		if (m_refcount == NULL)
			return;
		if ((*m_refcount)-- == 0) {
			delete m_data;
			delete m_refcount;
			m_data = NULL;
			m_refcount = NULL;
		}
	}
};

#endif
