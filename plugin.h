#ifndef _PLUGIN_H
#define _PLUGIN_H

#include <stdint.h>
#include <vector>
#include <stdio.h>

typedef int16_t sample_t;

struct audio_format {
	int samplerate;
	int channels;
};

class input_plugin {
public:
	virtual ~input_plugin() {}

	virtual int open(const char *fname) = 0;
	virtual int seek(int sec) { return -1; }
	virtual std::vector<sample_t> decode(audio_format *fmt) = 0;
};

class output_plugin {
public:
	virtual ~output_plugin() {}

	virtual int open(const audio_format &fmt) = 0;
	virtual ssize_t play(const audio_format &fmt,
			     const std::vector<sample_t> &buf) = 0;
};


#endif
