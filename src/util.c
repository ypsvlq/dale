#include <stdnoreturn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void *xmalloc(size_t n) {
	void *p = malloc(n);
	if (!p)
		err("Out of memory");
	return p;
}

void *xrealloc(void *p, size_t n) {
	void *p2 = realloc(p, n);
	if (!p2)
		err("Out of memory");
	return p2;
}

char *xstrdup(const char *s) {
	size_t n = strlen(s) + 1;
	char *s2 = xmalloc(n);
	return memcpy(s2, s, n);
}
