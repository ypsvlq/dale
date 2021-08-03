#include <string.h>
#include <stdlib.h>
#include "dale.h"

static int qsortstr(const void *a, const void *b) {
	return strcmp(*(const char**)a, *(const char**)b);
}

static bool match(char *pattern, const char *str) {
	char *p, *p2, *p3;
	size_t sz;
	bool greedy;
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
			greedy = true;
			if ((p = strchr(pattern, '*'))) {
				*p = 0;
				p2 = p + 1;
				while ((p3 = strchr(p2, '*'))) {
					*p3 = 0;
					if (!strcmp(pattern, p2)) {
						greedy = false;
						break;
					}
					*p3 = '*';
					p2 = p3 + 1;
				}
				if (!strcmp(pattern, p2))
					greedy = false;
			}
			str = strstr(str, pattern);
			if (!str) {
				if (p)
					*p = '*';
				return false;
			}
			if (greedy) {
				sz = strlen(pattern);
				while (sz && strlen(str) > sz) {
					p2 = strstr(str + sz, pattern);
					if (p2)
						str = p2;
					else
						break;
				}
			}
			if (p)
				*p = '*';
		}
	}
}

void glob(char *pattern, vec(char*) *out) {
	char *mpattern, *dir, *dirend, *nextdir, *file, *path, *path2;
	void *d;
	vec(char*) vec;

	mpattern = strchr(pattern, '*');
	if (!mpattern) {
		if (hostfexists(pattern))
			vec_push(*out, xstrdup(pattern));
		return;
	}

	dirend = strchr(pattern, '/');
	if (dirend && dirend < mpattern) {
		while ((nextdir = strchr(dirend+1, '/')) < mpattern && nextdir > dirend)
			dirend = nextdir;
		mpattern = dirend + 1;
		*dirend = 0;
	}
	dir = (dirend && !strchr(pattern, '*')) ? pattern : ".";
	d = hostdopen(dir);
	if (!d)
		return;

	if ((nextdir = strchr(mpattern, '/')))
		*nextdir = 0;

	vec = *out;

	while ((file = hostdread(d))) {
		if (match(mpattern, file)) {
			asprintf(&path, "%s/%s", dir, file);
			if (!nextdir) {
				vec_push(vec, path);
			} else {
				if (hostisdir(path)) {
					asprintf(&path2, "%s/%s", path, nextdir+1);
					glob(path2, &vec);
					free(path2);
				}
				free(path);
			}
		}
		free(file);
	}

	hostdclose(d);

	if (dirend)
		*dirend = '/';
	if (nextdir)
		*nextdir = '/';

	qsort(vec+vec_size(out), vec_size(vec)-vec_size(out), sizeof(*vec), qsortstr);
	*out = vec;
}
