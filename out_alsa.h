#ifndef _OUT_ALSA_H
#define _OUT_ALSA_H

#include "plugin.h"
#include <alsa/asoundlib.h>

class alsa_output: public output_plugin {
public:
	alsa_output();
	~alsa_output();

	int open(const audio_format &fmt);
	ssize_t play(const audio_format &fmt, const std::vector<sample_t> &buf);

private:
	snd_pcm_t *m_pcm;
};

#endif
