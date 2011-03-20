#ifndef _PLUGIN_H
#define _PLUGIN_H

#include <stdint.h>
#include <vector>

typedef int16_t sample_t;

class input_plugin {
public:
	virtual ~input_plugin() {}

	virtual int open(const char *fname) = 0;
	virtual int seek(int sec) { return -1; }
	virtual std::vector<sample_t> decode() = 0;
};

#endif
