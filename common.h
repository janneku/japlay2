#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>
#include <vector>

typedef int16_t sample_t;

void error(const char *fmt, ...);
void warning(const char *fmt, ...);

class input_plugin {
public:
	virtual ~input_plugin() {}

	virtual int open(const char *fname) = 0;
	virtual int seek(int msec) { return -1; }
	virtual std::vector<sample_t> decode() = 0;
};

#endif
