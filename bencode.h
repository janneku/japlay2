#ifndef _BENCODE_H
#define _BENCODE_H

#include "variant.h"
#include <string>
#include <assert.h>

class bdecoder {
public:
	bdecoder(const std::string &buf);
	~bdecoder();

	const char *error() const { return m_error; }
	int pos() const { return m_pos; }
	bool at_end() const { return m_pos == m_len; }

	void set_error(const char *fmt, ...);

	int decode(variant *value);

	static int decode_all(variant *value, const std::string &buf)
	{
		bdecoder decoder(buf);
		if (decoder.decode(value))
			return -1;
		if (!decoder.at_end())
			return -1;
		return 0;
	}

private:
	std::string m_buf;
	size_t m_pos;
	size_t m_len;
	int m_depth;
	char *m_error;

	int decode_string(std::string *s);
	bool validate_int(const std::string &s);
};

int bencode(std::string *out, const variant &value);

#endif
