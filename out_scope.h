#ifndef _OUT_SCOPE_H
#define _OUT_SCOPE_H

#include "plugin.h"
#include <gtkmm/window.h>

class scope: public Gtk::Window {
public:
	scope();

	void add(const audio_format &fmt, const std::vector<sample_t> &buf);

private:
	std::vector<sample_t> m_wave;

	bool on_expose_event(GdkEventExpose *event);
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
