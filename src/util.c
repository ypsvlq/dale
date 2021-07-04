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
	if (line)
		fprintf(stderr, "%s line %zu: ", fname, line);
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
	return memcpy(xmalloc(n), s, n);
}

char *xstrndup(const char *s, size_t max) {
	size_t n = strlen(s) + 1;
	if (max < n)
		n = max;
	return memcpy(xmalloc(n), s, n);
}

int asprintf(char **outp, const char *fmt, ...) {
	va_list ap, ap2;
	int n;
	va_start(ap, fmt);
	va_copy(ap2, ap);
	n = vsnprintf(NULL, 0, fmt, ap2);
	*outp = xmalloc(n + 1);
	vsprintf(*outp, fmt, ap);
	va_end(ap2);
	va_end(ap);
	return n;
}
