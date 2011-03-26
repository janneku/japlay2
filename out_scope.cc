#include "out_scope.h"
#include "common.h"

static double complex_abs(fftw_complex c)
{
	return sqrtf(c[0] * c[0] + c[1] * c[1]);
}

scope::scope() :
	m_peak(0), m_freq(false), m_dft(NULL)
{
	memset(m_wave, 0, sizeof m_wave);

	set_default_size(SAMPLES, 128);
	set_title("japlay2 scope");

	add_events(Gdk::BUTTON_PRESS_MASK);

	/* hanning window */
	for (size_t i = 0; i < SAMPLES; ++i)
		m_window[i] = 0.5 * (1.0 - cosf(2 * M_PI * i / SAMPLES));

	realize();
	Glib::RefPtr<Gdk::Window> window = get_window();
	window->set_background(Gdk::Color("Black"));
}

scope::~scope()
{
	if (m_dft)
		fftw_destroy_plan(m_dft);
}

void scope::add(const audio_format &fmt, const std::vector<sample_t> &buf)
{
	size_t frames = buf.size() / fmt.channels;
	if (frames < SAMPLES)
		return;
	if (fmt.channels == 1) {
		size_t j = frames - SAMPLES;
		for (size_t i = 0; i < SAMPLES; ++i)
			m_wave[i] = buf[j++] / 32768.0;
	} else if (fmt.channels == 2) {
		/* convert to mono by taking average of both channels */
		size_t j = frames - SAMPLES;
		for (size_t i = 0; i < SAMPLES; ++i) {
			m_wave[i] = (buf[j*2] + buf[j*2+1]) / (32768.0 * 2);
			j++;
		}
	}

	if (m_freq) {
		for (size_t i = 0; i < SAMPLES; ++i)
			m_wave[i] *= m_window[i];
		if (m_dft == NULL) {
			m_dft = fftw_plan_dft_r2c_1d(SAMPLES, m_wave, m_complex,
						     FFTW_ESTIMATE);
		}
		fftw_execute(m_dft);
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
		energy += (fabs(m_wave[i] - m_wave[i - 1]) + fabs(m_wave[i]) / 2);
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
		if (m_freq)
			cr->line_to(i, 120 - complex_abs(m_complex[i / 2]) * 10);
		else
			cr->line_to(i, m_wave[i] * 64 + 64);
	}
	cr->stroke();
	return false;
}

bool scope::on_button_press_event(GdkEventButton *event)
{
	m_freq = !m_freq;
	return true;
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
