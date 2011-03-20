#include "in_mad.h"
#include <assert.h>

mad_input::mad_input() :
	m_file(NULL), m_bitrate(128000)
{
	mad_frame_init(&m_frame);
	mad_stream_init(&m_stream);
	mad_synth_init(&m_synth);
}

mad_input::~mad_input()
{
	if (m_file)
		fclose(m_file);
	mad_frame_finish(&m_frame);
	mad_stream_finish(&m_stream);
	mad_synth_finish(&m_synth);
}

int mad_input::open(const char *fname)
{
	assert(m_file == NULL);
	m_file = fopen(fname, "rb");
	if (m_file == NULL)
		return -1;
	fill_readbuf();
	return 0;
}

int mad_input::fill_readbuf()
{
	if (m_stream.next_frame) {
		const char *p =
			reinterpret_cast<const char *>(m_stream.next_frame);
		size_t pos = p - m_readbuf.data();
		m_readbuf = m_readbuf.substr(pos);
	}

	size_t toread = 16384 - m_readbuf.size();

	std::string buf(toread, 0);
	ssize_t ret = fread(&buf[0], 1, toread, m_file);
	if (ret <= 0)
		return -1;
	buf.resize(ret);
	m_readbuf += buf;

	mad_stream_buffer(&m_stream,
		reinterpret_cast<const unsigned char *>(m_readbuf.data()),
		m_readbuf.size());

	return 0;
}

sample_t mad_input::scale(mad_fixed_t sample)
{
	/* round */
	sample += (1L << (MAD_F_FRACBITS - 16));

	/* clip */
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;

	/* quantize */
	return sample >> (MAD_F_FRACBITS + 1 - 16);
}

int mad_input::seek(int sec)
{
	ssize_t pos = ftell(m_file) - m_readbuf.size();
	if (m_stream.next_frame) {
		const char *p =
			reinterpret_cast<const char *>(m_stream.next_frame);
		pos += p - m_readbuf.data();
	}

	pos += sec * m_bitrate / 8;
	if (pos < 0) pos = 0;

	fseek(m_file, pos, SEEK_SET);

	mad_frame_finish(&m_frame);
	mad_stream_finish(&m_stream);
	mad_synth_finish(&m_synth);
	mad_frame_init(&m_frame);
	mad_stream_init(&m_stream);
	mad_synth_init(&m_synth);
	m_readbuf.clear();

	return fill_readbuf();
}

std::vector<sample_t> mad_input::decode(audio_format *fmt)
{
	while (1) {
		if (mad_header_decode(&m_frame.header, &m_stream)) {
			if (m_stream.error == MAD_ERROR_BUFLEN) {
				if (fill_readbuf())
					break;
				continue;
			} else if (MAD_RECOVERABLE(m_stream.error))
				continue;
			break;
		}

		if (mad_frame_decode(&m_frame, &m_stream)) {
			if (MAD_RECOVERABLE(m_stream.error))
				continue;
			break;
		}

		mad_synth_frame(&m_synth, &m_frame);

		m_bitrate = m_frame.header.bitrate;

		fmt->samplerate = m_frame.header.samplerate;
		fmt->channels = MAD_NCHANNELS(&m_frame.header);

		std::vector<sample_t> buf(m_synth.pcm.length * fmt->channels, 0);
		if (fmt->channels == 2) {
			for (size_t i = 0; i < m_synth.pcm.length; ++i) {
				buf[i*2] = scale(m_synth.pcm.samples[0][i]);
				buf[i*2+1] = scale(m_synth.pcm.samples[1][i]);
			}
		} else {
			for (size_t i = 0; i < m_synth.pcm.length; ++i) {
				buf[i] = scale(m_synth.pcm.samples[0][i]);
			}
		}
		return buf;
	}
	return std::vector<sample_t>();
}
