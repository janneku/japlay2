#ifndef _PLUGIN_H
#define _PLUGIN_H

#include <stdint.h>
#include <vector>
#include <stdio.h>

typedef int16_t sample_t;

class input_plugin {
public:
	virtual ~input_plugin() {}

	virtual int open(const char *fname) = 0;
	virtual int seek(int sec) { return -1; }
	virtual std::vector<sample_t> decode() = 0;
};

class output_plugin {
public:
	virtual ~output_plugin() {}

	virtual int open() = 0;
	virtual ssize_t play(const std::vector<sample_t> &buf) = 0;
};


#endif
