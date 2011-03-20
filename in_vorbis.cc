#include "in_vorbis.h"
#include <string.h>
#include <assert.h>

vorbis_input::vorbis_input() :
	m_file(NULL)
{
}

vorbis_input::~vorbis_input()
{
	if (m_file) {
		ov_clear(&m_vf);
		fclose(m_file);
	}
}

size_t vorbis_input::read_cb(void *ptr, size_t size, size_t nmemb,
			     void *datasource)
{
	vorbis_input *input = reinterpret_cast<vorbis_input *>(datasource);
	return fread(ptr, size, nmemb, input->m_file);
}

int vorbis_input::seek_cb(void *datasource, ogg_int64_t off, int whence)
{
	vorbis_input *input = reinterpret_cast<vorbis_input *>(datasource);
	return fseek(input->m_file, off, whence);
}

long vorbis_input::tell_cb(void *datasource)
{
	vorbis_input *input = reinterpret_cast<vorbis_input *>(datasource);
	return ftell(input->m_file);
}

int vorbis_input::open(const char *fname)
{
	static ov_callbacks callbacks;
	memset(&callbacks, 0, sizeof callbacks);
	callbacks.read_func = &vorbis_input::read_cb;
	callbacks.seek_func = &vorbis_input::seek_cb;
	callbacks.tell_func = &vorbis_input::tell_cb;

	assert(m_file == NULL);
	m_file = fopen(fname, "rb");
	if (m_file == NULL)
		return -1;
	if (ov_open_callbacks(this, &m_vf, NULL, 0, callbacks))
		return -1;
	return 0;
}

int vorbis_input::seek(int sec)
{
	if (ov_time_seek(&m_vf, ov_time_tell(&m_vf) + sec))
		return -1;
	return 0;
}

std::vector<sample_t> vorbis_input::decode(audio_format *fmt)
{
	while (1) {
		const size_t toread = 4096;
		std::vector<sample_t> buf(toread, 0);
		int bitstream;
		long ret = ov_read(&m_vf, reinterpret_cast<char *>(&buf[0]),
				   toread * 2, 0, 2, 1, &bitstream);
		if (ret == OV_HOLE)
			continue;
		if (ret <= 0)
			break;
		buf.resize(ret / 2);

		vorbis_info *vi = ov_info(&m_vf, -1);
		fmt->samplerate = vi->rate;
		fmt->channels = vi->channels;

		return buf;
	}
	return std::vector<sample_t>();
}
