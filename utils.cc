#include "utils.h"
#include <stdio.h>
#include <openssl/sha.h>

std::string calc_sha1(const std::string &buf)
{
	unsigned char digest[SHA_DIGEST_LENGTH];
	const unsigned char *d =
		reinterpret_cast<const unsigned char *>(buf.data());
	SHA1(d, buf.size(), digest);
	return std::string(reinterpret_cast<char *>(digest), sizeof digest);
}

std::string load_file(const char *fname, size_t maxlen)
{
	FILE *f = fopen(fname, "rb");
	if (f == NULL)
		return "";
	if (maxlen == 0) {
		fseek(f, 0, SEEK_END);
		maxlen = ftell(f);
		fseek(f, 0, SEEK_SET);
	}

	std::string buf(maxlen, 0);
	ssize_t ret = fread(&buf[0], 1, maxlen, f);
	fclose(f);
	if (ret < 0)
		return "";
	buf.resize(ret);
	return buf;
}

int write_file(const char *fname, const std::string &buf)
{
	FILE *f = fopen(fname, "wb");
	if (f == NULL)
		return -1;
	if (fwrite(buf.data(), 1, buf.size(), f) < buf.size()) {
		fclose(f);
		return -1;
	}
	fclose(f);
	return 0;
}

std::string hexlify(const std::string &buf)
{
	const char hex[] = "0123456789abcdef";
	std::string out;
	for (size_t i = 0; i < buf.size(); ++i) {
		out += hex[(buf[i] >> 4) & 0xf];
		out += hex[buf[i] & 0xf];
	}
	return out;
}
