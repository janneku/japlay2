#include "out_alsa.h"

alsa_output::alsa_output() :
	m_pcm(NULL)
{
}

alsa_output::~alsa_output()
{
	if (m_pcm)
		snd_pcm_close(m_pcm);
}

int alsa_output::open()
{
	if (snd_pcm_open(&m_pcm, "default", SND_PCM_STREAM_PLAYBACK, 0))
		return -1;
	if (snd_pcm_set_params(m_pcm, SND_PCM_FORMAT_S16_LE,
			SND_PCM_ACCESS_RW_INTERLEAVED, 2, 44100, 1,
			200000))
		return -1;
	return 0;
}

ssize_t alsa_output::play(const std::vector<sample_t> &buf)
{
	while (1) {
		snd_pcm_sframes_t ret =
			snd_pcm_writei(m_pcm, buf.data(), buf.size() / 2);
		if (ret < 0) {
			if (snd_pcm_recover(m_pcm, ret, 1) == 0)
				continue;
			break;
		}
		return ret * 2;
	}
	return -1;
}
