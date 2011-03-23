#ifndef _COMMON_H
#define _COMMON_H

void error(const char *fmt, ...);
void warning(const char *fmt, ...);

class ui_lock {
public:
	ui_lock();
	~ui_lock();

private:
	bool m_locked;
};

#endif
