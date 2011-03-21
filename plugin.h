#ifndef _PLUGIN_H
#define _PLUGIN_H

#include "smart_ptr.h"
#include <string>
#include <stdint.h>
#include <vector>
#include <stdio.h>

typedef int16_t sample_t;

struct song {
	std::string title;
	std::string fname;
	std::string sha1;
	int rating;

	int adjust(int d);

	song() :
		rating(5)
	{
	}
	int load();
	int save();
};

struct audio_format {
	int samplerate;
	int channels;
};

class input_plugin {
public:
	virtual ~input_plugin() {}

	virtual int open(song *song) = 0;
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
