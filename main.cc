#include "in_mad.h"
#include "bencode.h"
#include "utils.h"
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

struct song: public Glib::Object {
	std::string title;
	std::string fname;
	std::string sha1;
	int rating;

	int adjust(int d);

	song() :
		rating(5)
	{
	}
	int load();
	int save();
};

class playlist_columns: public Gtk::TreeModel::ColumnRecord {
public:
	playlist_columns()
	{
		add(title);
		add(rating);
		add(songptr);
	}

	Gtk::TreeModelColumn<Glib::ustring> title;
	Gtk::TreeModelColumn<Glib::ustring> rating;
	Gtk::TreeModelColumn<Glib::RefPtr<song> > songptr;
};

class player: public Gtk::Window {
public:
	player();

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
	void on_current_changed();
};

namespace {

Glib::Mutex *play_mutex = NULL;
Glib::Cond *play_cond = NULL;
Glib::RefPtr<song> current;
Gtk::TreeModel::iterator current_iter;
int toseek = 0;
bool reset = false;
enum state_enum {
	STOP,
	PAUSED,
	PLAYING,
} state = STOP;
Glib::RefPtr<Gtk::ListStore> playlist;
sigc::signal<void> current_changed;
std::string db_path;
}

playlist_columns playlist_columns;

int song::adjust(int d)
{
	rating += d;
	rating = std::min(10, std::max(0, rating));
	return save();
}

int song::load()
{
	std::string fname = db_path + '/' + hexlify(sha1);
	std::string buf = load_file(fname.c_str());
	if (buf.empty())
		return -1;

	variant v;
	variant_map d;
	if (bdecoder::decode_all(&v, buf) ||
	    v.get(&d) ||
	    d.get("rating").get(&rating)) {
		warning("Corrupted file: %s", fname.c_str());
		return -1;
	}

	return 0;
}

int song::save()
{
	variant_map d;
	d.insert("rating", rating);

	std::string buf;
	bencode(&buf, d);

	std::string fname = db_path + '/' + hexlify(sha1);
	return write_file(fname.c_str(), buf);
}

void set_current(Gtk::TreeModel::iterator iter)
{
	if (current && current_iter) {
		Gtk::TreeModel::Row row = *current_iter;
		char stars[11];
		stars[10] = 0;
		memset(stars, '*', current->rating);
		memset(&stars[current->rating], '-', 10-current->rating);
		row[playlist_columns.rating] = stars;
	}
	Gtk::TreeModel::Row row = *iter;
	current = row.get_value(playlist_columns.songptr);
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

	current_changed
		.connect(sigc::mem_fun(*this, &player::on_current_changed));

	m_buttons.pack_start(m_play);
	m_buttons.pack_start(m_stop);
	m_buttons.pack_start(m_pause);
	m_buttons.pack_start(m_previous);
	m_buttons.pack_start(m_next);

	m_playlist_view.set_model(playlist);
	m_playlist_view.append_column("Rating", playlist_columns.rating);
	m_playlist_view.append_column("Title", playlist_columns.title);

	m_scrolledwin.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	m_scrolledwin.add(m_playlist_view);

	m_layout.pack_start(m_buttons, Gtk::PACK_SHRINK);
	m_layout.pack_start(m_scrolledwin);

	add(m_layout);
	show_all();
}

void player::play_clicked()
{
	Gtk::TreeModel::iterator i
		= m_playlist_view.get_selection()->get_selected();
	if (!i)
		return;
	Glib::Mutex::Lock lock(*play_mutex);
	if (current)
		current->adjust((state == PLAYING) ? -2 : -1);
	set_current(i);
	on_current_changed();
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
	current->adjust((state == PLAYING) ? -2 : -1);
	set_current(i);
	on_current_changed();
	play_cond->signal();
}

void player::next_clicked()
{
	Glib::Mutex::Lock lock(*play_mutex);
	if (!current_iter)
		return;
	Gtk::TreeIter i = current_iter;
	i++;
	if (!i)
		return;
	current->adjust((state == PLAYING) ? -2 : -1);
	set_current(i);
	on_current_changed();
	play_cond->signal();
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

void player::on_current_changed()
{
	set_title(current->title);
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
	input_plugin *input = NULL;

	ao_initialize();

	std::vector<sample_t> playbuf;
	size_t played = 0;

	while (1) {
		Glib::Mutex::Lock lock(*play_mutex);

		if (reset || state == STOP) {
			toseek = 0;
			playbuf.clear();
			delete input;
			input = NULL;
			reset = false;
			played = 0;
		}
		if (state != PLAYING) {
			play_cond->wait(*play_mutex);
			continue;
		}

		if (input == NULL) {
			input = new mad_input;
			if (input->open(current->fname.c_str())) {
				warning("Unable to open file");
				state = STOP;
				continue;
			}
			current->adjust(1);
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

		const int RATING_SEC = 20;

		if (played >= format.rate * RATING_SEC * 2) {
			played -= format.rate * RATING_SEC * 2;
			current->adjust(1);
		}

		lock.release();
		std::vector<sample_t> buf = input->decode();
		if (buf.empty()) {
			ao_play(dev, reinterpret_cast<char *>(&playbuf[0]),
				playbuf.size() * 2);
			playbuf.clear();

			lock.acquire();
			current->adjust(1);
			if (current_iter) {
				Gtk::TreeIter i = current_iter;
				i++;
				if (i) {
					set_current(i);
					gdk_threads_enter();
					current_changed.emit();
					gdk_threads_leave();
				} else
					state = STOP;
			} else
				state = STOP;
			continue;
		}

		played += buf.size();

		playbuf.insert(playbuf.end(), buf.begin(), buf.end());
		if (playbuf.size() >= format.rate / 20 * 2) {
			ao_play(dev, reinterpret_cast<char *>(&playbuf[0]),
				playbuf.size() * 2);
			playbuf.clear();
		}
	}
}

void add_file(const std::string &fname)
{
	std::string buf = load_file(fname.c_str(), 4096);
	std::string sha1;
	if (!buf.empty())
		sha1 = calc_sha1(buf);

	size_t p = fname.rfind('/');

	Glib::RefPtr<song> song(new class song);
	song->title = fname.substr(p + 1);
	song->fname = fname;
	song->sha1 = sha1;
	song->load();

	char stars[11];
	stars[10] = 0;
	memset(stars, '*', song->rating);
	memset(&stars[song->rating], '-', 10-song->rating);

	Gtk::TreeModel::Row row = *(playlist->append());
	row[playlist_columns.title] = fname.substr(p + 1);
	row[playlist_columns.rating] = stars;
	row[playlist_columns.songptr] = song;
}

int scan_directory(const std::string &path)
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
		add_file(fname);
	}
	closedir(d);
	return 0;
}

}

int main(int argc, char **argv)
{
	Glib::thread_init();
	Gtk::Main kit(argc, argv);

	db_path = std::string(getenv("HOME")) + "/.japlay2";
	if (mkdir(db_path.c_str(), 0755) && errno != EEXIST)
		warning("Unable to create db directory (%s)", strerror(errno));

	play_mutex = new Glib::Mutex;
	play_cond = new Glib::Cond;

	playlist = Gtk::ListStore::create(playlist_columns);
	for (int i = 1; i < argc; ++i)
		scan_directory(argv[i]);

	Glib::Thread::create(&play_thread, true);

	player player;
	gdk_threads_enter();
	Gtk::Main::run(player);
	gdk_threads_leave();

	return 0;
}
