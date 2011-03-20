#ifndef _UTILS_H
#define _UTILS_H

#include <string>

std::string calc_sha1(const std::string &buf);
std::string load_file(const char *fname, size_t maxlen = 0);
int write_file(const char *fname, const std::string &buf);
std::string hexlify(const std::string &buf);

#endif
