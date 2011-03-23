#include "out_scope.h"
#include "common.h"

const size_t SAMPLES = 512;

scope::scope() :
	m_wave(SAMPLES, 0)
{
	set_default_size(SAMPLES, 128);
	set_title("japlay2 scope");
}

void scope::add(const audio_format &fmt, const std::vector<sample_t> &buf)
{
	size_t frames = buf.size() / fmt.channels;
	if (frames < SAMPLES)
		return;
	if (fmt.channels == 1) {
		memcpy(&m_wave[0], &buf[buf.size() - SAMPLES], SAMPLES * 2);
	} else if (fmt.channels == 2) {
		/* convert to mono by taking average of both channels */
		size_t j = frames - SAMPLES;
		for (size_t i = 0; i < SAMPLES; ++i) {
			m_wave[i] = (buf[j*2] + buf[j*2+1]) / 2;
			j++;
		}
	}
	queue_draw();
}

bool scope::on_expose_event(GdkEventExpose *event)
{
	Glib::RefPtr<Gdk::Window> window = get_window();
	Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context();
	cr->rectangle(event->area.x, event->area.y,
		      event->area.width, event->area.height);
	cr->clip();

	cr->set_source_rgb(0, 0, 0);
	cr->move_to(0, 128);
	for (size_t i = 0; i < SAMPLES; ++i) {
		cr->line_to(i, m_wave[i] / 512 + 64);
	}
	cr->stroke();
	return false;
}

scope_output::scope_output()
{
	m_scope = new scope;
	m_scope->show();
}

scope_output::~scope_output()
{
	delete m_scope;
}

int scope_output::set_format(const audio_format &fmt)
{
	return 0;
}

ssize_t scope_output::play(const audio_format &fmt,
			  const std::vector<sample_t> &buf)
{
	ui_lock l;
	m_scope->add(fmt, buf);
	return 0;
}
