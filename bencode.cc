#include "bencode.h"
#include <cctype>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>

const int MAX_DEPTH = 64;

/*
 * COMPABILITY NOTE
 *
 * Please note that the original decoder in bencode.py accepts integers with
 * leading spaces and zeroes. To enable compability with the original decoder,
 * comment out the following lines.
 */
#define NO_LEADING_SPACES
#define NO_LEADING_ZEROES

static int bencode_string(std::string *out, const std::string &s)
{
	char length[16];
	sprintf(length, "%zd:", s.size());
	*out += length + s;
	return 0;
}

int bencode(std::string *out, const variant &value)
{
	variant_map dict;
	variant_list list;
	int i;
	long long ll;
	bool b;
	std::string s;

	if (value.get(&dict) == 0) {
		*out += 'd';
		for (map_const_iter<std::string, variant> i(dict); i.valid();
		     i.next()) {
			bencode_string(out, i.key());
			if (bencode(out, *i))
				return -1;
		}
		*out += 'e';

	} else if (value.get(&list) == 0) {
		*out += 'l';
		for (list_const_iter<variant> i(list); i.valid(); i.next()) {
			if (bencode(out, *i))
				return -1;
		}
		*out += 'e';

	} else if (value.get(&i) == 0) {
		char number[16];
		sprintf(number, "i%de", i);
		*out += number;

	} else if (value.get(&ll) == 0) {
		char number[22];
		sprintf(number, "i%llde", ll);
		*out += number;

	} else if (value.get(&b) == 0) {
		*out += b ? "b1" : "b0";

	} else if (value.get(&s) == 0) {
		bencode_string(out, s);

	} else {
		fprintf(stderr, "Unable to bencode\n");
		return -1;
	}

	return 0;
}

int bdecoder::decode_string(std::string *s)
{
	size_t end = m_buf.find(':', m_pos);
	if (end == std::string::npos || end >= m_len) {
		set_error("Unterminated string length");
		return -1;
	}
	std::string length = m_buf.substr(m_pos, end - m_pos);
	if (!validate_int(length))
		return -1;

	char *endp = NULL;
	size_t strlen = strtoul(length.c_str(), &endp, 10);
	if (size_t(endp - length.c_str()) != length.size()) {
		set_error("Invalid string length (%s)", length.c_str());
		return -1;
	}
	m_pos = end + 1;

	if (strlen + m_pos > m_len) {
		set_error("Input overrun");
		return -1;
	}
	*s = m_buf.substr(m_pos, strlen);
	m_pos += strlen;
	return 0;
}

bdecoder::bdecoder(const std::string &buf) :
	m_buf(buf), m_pos(0), m_len(buf.size()), m_depth(0), m_error(NULL)
{
}

bdecoder::~bdecoder()
{
	free(m_error);
}

int bdecoder::decode(variant *value)
{
	if (m_pos >= m_len) {
		set_error("Input overrun");
		return -1;
	}
	if (m_depth >= MAX_DEPTH) {
		set_error("Recursion limit exceeded");
		return -1;
	}
	m_depth++;
	switch (m_buf[m_pos]) {
	case 'd':{
			m_pos++;
			variant_map dict;
			while (m_buf[m_pos] != 'e') {
				std::string key;
				variant item;
				if (decode_string(&key) ||
				    decode(&item))
					return -1;
				dict.insert(key, item);
			}
			m_pos++;
			*value = dict;
		}
		break;

	case 'l':{
			m_pos++;
			variant_list list;
			while (m_buf[m_pos] != 'e') {
				variant item;
				if (decode(&item))
					return -1;
				list.push_back(item);
			}
			m_pos++;
			*value = list;
		}
		break;

	case 'i':{
			m_pos++;
			size_t end = m_buf.find('e', m_pos);
			if (end == std::string::npos || end >= m_len) {
				set_error("Unterminated integer");
				return -1;
			}

			std::string number = m_buf.substr(m_pos, end - m_pos);
			if (!validate_int(number))
				return -1;

			char *endp = NULL;
			long long ll = strtoll(number.c_str(), &endp, 10);
			if (size_t(endp - number.c_str()) != number.size()) {
				set_error("Invalid integer (%s)",
					  number.c_str());
				return -1;
			}
			if (ll >= INT_MIN && ll <= INT_MAX)
				*value = int(ll);
			else
				*value = ll;
			m_pos = end + 1;
		}
		break;

	case 'b':{
			m_pos++;
			switch (m_buf[m_pos]) {
			case '1':
				*value = true;
				break;
			case '0':
				*value = false;
				break;
			default:
				set_error("Invalid boolean");
				return -1;
			}
			m_pos++;
		}
		break;

	default: {
			std::string s;
			if (decode_string(&s))
				return -1;
			*value = s;
		}
		break;
	}
	assert(m_depth > 0);
	m_depth--;
	return 0;
}

void bdecoder::set_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vasprintf(&m_error, fmt, ap);
	va_end(ap);
}

bool bdecoder::validate_int(const std::string &s)
{
#ifdef NO_LEADING_SPACES
	if (s.size() >= 1 && isspace(s[0])) {
		set_error("Integer with leading spaces: %s", s.c_str());
		return false;
	}
#endif
#ifdef NO_LEADING_ZEROES
	if (s.size() >= 2 && (s[0] == '0' || (s[0] == '-' && s[1] == '0'))) {
		set_error("Integer with leading zeroes: %s", s.c_str());
		return false;
	}
#endif
	return true;
}
