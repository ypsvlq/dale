#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
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
	hostquit();
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
	size_t n;
	char *p;
	n = strlen(s);
	if (max < n)
		n = max;
	p = memcpy(xmalloc(n + 1), s, n);
	p[n] = 0;
	return p;
}

int asprintf(char **outp, const char *fmt, ...) {
	va_list ap, ap2;
	int n;
	char *p;
	va_start(ap, fmt);
	va_copy(ap2, ap);
	n = vsnprintf(NULL, 0, fmt, ap2);
	p = xmalloc(n + 1);
	vsprintf(p, fmt, ap);
	*outp = p;
	va_end(ap2);
	va_end(ap);
	return n;
}

char *rstrtok(char **sp, const char *delim) {
	char *s, *end;

	s = *sp;
	if (!s)
		return NULL;

	s += strspn(s, delim);
	if (!*s) {
		*sp = 0;
		return NULL;
	}
	end = s + strcspn(s, delim);
	if (*end)
		*end++ = 0;
	else
		end = 0;

	*sp = end;
	return s;
}

static uintmax_t lstrtou(const char *s, uintmax_t max) {
	uintmax_t um;
	char *endp;
	errno = 0;
	um = strtoumax(s, &endp, 0);
	if (errno == ERANGE || um > max)
		err("Overflow converting '%s' to int", s);
	if (*endp)
		fprintf(stderr, "Warning: Extra data when converting '%s' to int\n", s);
	return um;
}

size_t atosz(const char *s) {
	return lstrtou(s, SIZE_MAX);
}
