#include "common.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

void error(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	char *msg;
	vasprintf(&msg, fmt, argp);
	va_end(argp);
	fprintf(stderr, "ERROR: %s\n", msg);
	exit(1);
	free(msg);
}

void warning(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	char *msg;
	vasprintf(&msg, fmt, argp);
	va_end(argp);
	fprintf(stderr, "WARNING: %s\n", msg);
	free(msg);
}
