#include "in_mad.h"
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/box.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>
#include <dirent.h>
#include <ao/ao.h>

class playlist_columns: public Gtk::TreeModel::ColumnRecord {
public:
	playlist_columns()
	{
		add(title);
		add(filename);
	}

	Gtk::TreeModelColumn<Glib::ustring> title;
	Gtk::TreeModelColumn<Glib::ustring> filename;
};

class player: public Gtk::Window {
public:
	player();

	int scan_directory(const std::string &path);

private:
	Gtk::Button m_play;
	Gtk::Button m_stop;
	Gtk::Button m_pause;
	Gtk::Button m_previous;
	Gtk::Button m_next;
	Gtk::HBox m_buttons;
	Gtk::VBox m_layout;
	Gtk::ScrolledWindow m_scrolledwin;
	Gtk::TreeView m_playlist_view;

	void play_clicked();
	void stop_clicked();
	void pause_clicked();
	void previous_clicked();
	void next_clicked();
	bool on_key_press_event(GdkEventKey *event);
};

namespace {

Glib::Mutex *play_mutex = NULL;
Glib::Cond *play_cond = NULL;
std::string current;
Gtk::TreeModel::iterator current_iter;
int toseek = 0;
bool reset = false;
enum state_enum {
	STOP,
	PAUSED,
	PLAYING,
} state = STOP;
Glib::RefPtr<Gtk::ListStore> playlist;

}

playlist_columns playlist_columns;

void set_current(Gtk::TreeModel::iterator iter)
{
	Gtk::TreeModel::Row row = *iter;
	current = row.get_value(playlist_columns.filename).raw();
	current_iter = iter;
	reset = true;
}

player::player() :
	m_play(Gtk::Stock::MEDIA_PLAY),
	m_stop(Gtk::Stock::MEDIA_STOP),
	m_pause(Gtk::Stock::MEDIA_PAUSE),
	m_previous(Gtk::Stock::MEDIA_PREVIOUS),
	m_next(Gtk::Stock::MEDIA_NEXT)
{
	m_play.signal_clicked()
		.connect(sigc::mem_fun(*this, &player::play_clicked));
	m_stop.signal_clicked()
		.connect(sigc::mem_fun(*this, &player::stop_clicked));
	m_pause.signal_clicked()
		.connect(sigc::mem_fun(*this, &player::pause_clicked));
	m_previous.signal_clicked()
		.connect(sigc::mem_fun(*this, &player::previous_clicked));
	m_next.signal_clicked()
		.connect(sigc::mem_fun(*this, &player::next_clicked));

	signal_key_press_event()
		.connect(sigc::mem_fun(*this, &player::on_key_press_event));

	m_buttons.pack_start(m_play);
	m_buttons.pack_start(m_stop);
	m_buttons.pack_start(m_pause);
	m_buttons.pack_start(m_previous);
	m_buttons.pack_start(m_next);

	m_playlist_view.set_model(playlist);
	m_playlist_view.append_column("Title", playlist_columns.title);

	m_scrolledwin.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	m_scrolledwin.add(m_playlist_view);

	m_layout.pack_start(m_buttons, Gtk::PACK_SHRINK);
	m_layout.pack_start(m_scrolledwin);

	add(m_layout);
	show_all();
}

int player::scan_directory(const std::string &path)
{
	DIR *d = opendir(path.c_str());
	if (d == NULL)
		return -1;
	while (1) {
		struct dirent *ent = readdir(d);
		if (ent == NULL)
			break;
		if (ent->d_name[0] == '.') continue;

		const std::string fname = path + '/' + ent->d_name;
		if (scan_directory(fname) == 0)
			continue;

		Gtk::TreeModel::Row row = *(playlist->append());
		row[playlist_columns.title] = ent->d_name;
		row[playlist_columns.filename] = fname;
	}
	closedir(d);
	return 0;
}

void player::play_clicked()
{
	Gtk::TreeModel::iterator i
		= m_playlist_view.get_selection()->get_selected();
	if (!i)
		return;
	Glib::Mutex::Lock lock(*play_mutex);
	set_current(i);
	state = PLAYING;
	play_cond->signal();
}

void player::stop_clicked()
{
	Glib::Mutex::Lock lock(*play_mutex);
	state = STOP;
	play_cond->signal();
}

void player::pause_clicked()
{
	Glib::Mutex::Lock lock(*play_mutex);
	if (state == PLAYING)
		state = PAUSED;
	else if (state == PAUSED)
		state = PLAYING;
	play_cond->signal();
}

void player::previous_clicked()
{
	Glib::Mutex::Lock lock(*play_mutex);
	if (!current_iter || current_iter == playlist->children().begin())
		return;
	Gtk::TreeIter i = current_iter;
	i--;
	set_current(i);
	play_cond->signal();
}

void player::next_clicked()
{
	Glib::Mutex::Lock lock(*play_mutex);
	if (!current_iter)
		return;
	Gtk::TreeIter i = current_iter;
	i++;
	if (i) {
		set_current(i);
		play_cond->signal();
	}
}

bool player::on_key_press_event(GdkEventKey *event)
{
	Glib::Mutex::Lock lock(*play_mutex);
	if (state != PLAYING && state != PAUSED)
		return true;
	if (event->keyval == GDK_KEY_Left)
		toseek -= 10;
	if (event->keyval == GDK_KEY_Right)
		toseek += 10;
	if (event->keyval == GDK_KEY_Up)
		toseek += 60;
	if (event->keyval == GDK_KEY_Down)
		toseek -= 60;
	return true;
}

namespace {

void play_thread()
{
	ao_sample_format format;
	memset(&format, 0, sizeof format);
	format.bits = 16;
	format.byte_format = AO_FMT_NATIVE;
	format.rate = 44100;
	format.channels = 2;

	ao_device *dev = NULL;
	mad_input *input = NULL;

	ao_initialize();

	while (1) {
		Glib::Mutex::Lock lock(*play_mutex);

		if (reset || state == STOP) {
			toseek = 0;
			delete input;
			input = NULL;
			reset = false;
		}
		if (state != PLAYING) {
			play_cond->wait(*play_mutex);
			continue;
		}

		if (input == NULL) {
			input = new mad_input;
			if (input->open(current.c_str())) {
				warning("Unable to open file");
				state = STOP;
				continue;
			}
		}
		if (dev == NULL) {
			dev = ao_open_live(ao_default_driver_id(), &format,
					   NULL);
			if (dev == NULL) {
				warning("Unable to open audio device");
				state = STOP;
				continue;
			}
		}

		if (toseek) {
			if (input->seek(toseek)) {
				warning("Unable to seek");
			}
			toseek = 0;
		}

		lock.release();
		std::vector<sample_t> buf = input->decode();
		if (buf.empty()) {
			lock.acquire();
			if (current_iter) {
				Gtk::TreeIter i = current_iter;
				i++;
				if (i)
					set_current(i);
				else
					state = STOP;
			} else
				state = STOP;
			continue;
		}

		ao_play(dev, reinterpret_cast<char *>(&buf[0]), buf.size() * 2);
	}
}

}

int main(int argc, char **argv)
{
	Glib::thread_init();
	Gtk::Main kit(argc, argv);

	play_mutex = new Glib::Mutex;
	play_cond = new Glib::Cond;

	playlist = Gtk::ListStore::create(playlist_columns);

	Glib::Thread::create(&play_thread, true);

	player player;
	for (int i = 1; i < argc; ++i)
		player.scan_directory(argv[i]);
	Gtk::Main::run(player);

	return 0;
}
