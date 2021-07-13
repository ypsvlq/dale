#include <string.h>
#include <stdlib.h>
#include "dale.h"

static bool match(char *pattern, const char *str) {
	char *p, *p2;
	size_t sz;
	while (1) {
		if (!*pattern && !*str) {
			return true;
		} else if (!*pattern || !*str) {
			return false;
		} else if (*pattern == '?') {
			pattern++;
			str++;
		} else if (*pattern != '*') {
			if (*pattern != *str)
				return false;
			pattern++;
			str++;
		} else {
			pattern++;
			if (!*pattern)
				return true;
			if ((p = strchr(pattern, '*')))
				*p = 0;
			str = strstr(str, pattern);
			if (!str) {
				if (p)
					*p = '*';
				return false;
			}
			sz = strlen(pattern);
			while (sz && strlen(str) > sz) {
				p2 = strstr(str + sz, pattern);
				if (p2)
					str = p2;
				else
					break;
			}
			if (p)
				*p = '*';
		}
	}
}

void glob(char *pattern, char ***out, size_t *outsz) {
	char *p, *p2, *p3, *p4, *p5, *p6, *path;
	void *dir;

	p = strchr(pattern, '*');
	if (!p) {
		if (hostfexists(pattern)) {
			*out = xrealloc(*out, sizeof(**out) * ++*outsz);
			(*out)[*outsz - 1] = xstrdup(pattern);
		}
		return;
	}
	p2 = strchr(pattern, '/');
	if (p2 && p2 < p) {
		while ((p3 = strchr(p2+1, '/')) < p && p3 > p2) {
			dir = p2;
			p2 = p3;
		}
		p = p2 + 1;
		*p2 = 0;
	}
	path = p2 && !strchr(pattern, '*') ? pattern : ".";
	dir = hostdopen(path);
	if (!dir)
		return;

	if ((p3 = strchr(p, '/')))
		*p3 = 0;

	while ((p4 = hostdread(dir))) {
		if (match(p, p4)) {
			asprintf(&p5, "%s/%s", path, p4);
			if (!p3) {
				*out = xrealloc(*out, sizeof(**out) * ++*outsz);
				(*out)[*outsz - 1] = p5;
			} else {
				if (hostisdir(p5)) {
					asprintf(&p6, "%s/%s", p5, p3+1);
					glob(p6, out, outsz);
					free(p6);
				}
				free(p5);
			}
		}
		free(p4);
	}

	if (p2)
		*p2 = '/';
	if (p3)
		*p3 = '/';

	hostdclose(dir);
}
