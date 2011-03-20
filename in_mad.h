#ifndef _IN_MAD_H
#define _IN_MAD_H

#include "common.h"
#include <vector>
#include <string>
#include <stdio.h>
#include <mad.h>

class mad_input {
public:
	mad_input();
	~mad_input();

	int open(const char *fname);

	int seek(int msec);

	std::vector<sample_t> decode();

private:
	mad_stream m_stream;
	mad_frame m_frame;
	mad_synth m_synth;
	FILE *m_file;
	std::string m_readbuf;
	int m_bitrate;

	int fill_readbuf();
	sample_t scale(mad_fixed_t sample);
};

#endif
