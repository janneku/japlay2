#include "out_scope.h"
#include "common.h"

const size_t SAMPLES = 512;

scope::scope() :
	m_wave(SAMPLES, 0), m_peak(0)
{
	set_default_size(SAMPLES, 128);
	set_title("japlay2 scope");

	realize();
	Glib::RefPtr<Gdk::Window> window = get_window();
	window->set_background(Gdk::Color("Black"));
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

	float energy = 0;
	for (size_t i = 1; i < SAMPLES; ++i) {
		energy += (abs(m_wave[i] - m_wave[i - 1]) + abs(m_wave[i]) / 2)
			/ 65536.0;
	}
	energy *= 1.0 / SAMPLES;

	m_peak += (energy - m_peak) * 0.1;

	energy /= m_peak;

	float r = std::min(energy - 1, 1.0f);
	float g = std::max(std::min(3 - energy, 1.0f), 0.0f);

	cr->set_line_width(1);
	cr->set_source_rgb(r * 0.5, g * 0.5, 0);
	for (size_t i = 0; i < 4; ++i) {
		cr->move_to(0, i * 32);
		cr->line_to(SAMPLES, i * 32);
	}
	cr->stroke();

	cr->set_line_width(energy);
	cr->set_source_rgb(r, g, 0);
	cr->move_to(0, 64);
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
