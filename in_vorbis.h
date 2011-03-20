#ifndef _IN_VORBIS_H
#define _IN_VORBIS_H

#include "plugin.h"
#include <string>
#include <stdio.h>
#include <vorbis/vorbisfile.h>

class vorbis_input: public input_plugin {
public:
	vorbis_input();
	~vorbis_input();

	int open(const char *fname);
	int seek(int sec);
	std::vector<sample_t> decode(audio_format *fmt);

private:
	OggVorbis_File m_vf;
	FILE *m_file;

	static size_t read_cb(void *ptr, size_t size, size_t nmemb,
			      void *datasource);
	static int seek_cb(void *datasource, ogg_int64_t off, int whence);
	static long tell_cb(void *datasource);
};

#endif
