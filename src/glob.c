#include <string.h>
#include <stdlib.h>
#include "dale.h"

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

void glob(char *pattern, char ***out, size_t *outsz) {
	char *mpattern, *dir, *dirend, *nextdir, *file, *path, *path2;
	void *d;
	char **arr;
	size_t len;

	mpattern = strchr(pattern, '*');
	if (!mpattern) {
		if (hostfexists(pattern)) {
			*out = xrealloc(*out, sizeof(**out) * ++*outsz);
			(*out)[*outsz - 1] = xstrdup(pattern);
		}
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

	arr = *out;
	len = *outsz;

	while ((file = hostdread(d))) {
		if (match(mpattern, file)) {
			asprintf(&path, "%s/%s", dir, file);
			if (!nextdir) {
				arr = xrealloc(arr, sizeof(*arr) * ++len);
				arr[len - 1] = path;
			} else {
				if (hostisdir(path)) {
					asprintf(&path2, "%s/%s", path, nextdir+1);
					glob(path2, &arr, &len);
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

	qsort(arr+*outsz, len-*outsz, sizeof(*arr), qsortstr);
	*out = arr;
	*outsz = len;
}
