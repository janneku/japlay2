#ifndef _OUT_SCOPE_H
#define _OUT_SCOPE_H

#include "plugin.h"
#include <gtkmm/window.h>
#include <fftw3.h>

class scope: public Gtk::Window {
public:
	scope();
	~scope();

	void add(const audio_format &fmt, const std::vector<sample_t> &buf);

private:
	static const size_t SAMPLES = 512;
	double m_wave[SAMPLES];
	double m_window[SAMPLES];
	fftw_complex m_complex[SAMPLES/2 + 1];
	double m_peak;
	bool m_freq;
	fftw_plan m_dft;

	bool on_expose_event(GdkEventExpose *event);
	bool on_button_press_event(GdkEventButton *event);
};

class scope_output: public output_plugin {
public:
	scope_output();
	~scope_output();

	int set_format(const audio_format &fmt);
	ssize_t play(const audio_format &fmt, const std::vector<sample_t> &buf);

private:
	scope *m_scope;
};

#endif
