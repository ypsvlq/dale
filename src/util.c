#include <stdnoreturn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "dale.h"

noreturn void err(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "Error: ");
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
	exit(1);
}
