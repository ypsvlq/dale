#include <stdlib.h>
#include <string.h>
#include "dale.h"

static char *map(vec(char*));
static char *stripext(vec(char*));
static char *glob_(vec(char*));

const struct ebuiltin ebuiltins[] = {
	{"map", map, 1},
	{"stripext", stripext, 1},
	{"glob", glob_, 1},
	{0}
};

static char *map(vec(char*) args) {
	char *out = NULL;
	size_t len = 0;
	char *arr, *cur, *p, *ctx;
	size_t n;

	if (vec_size(args) < 2)
		err("Nothing to map");

	arr = xstrdup(varget(args[0]));
	ctx = arr;
	while ((cur = rstrtok(&ctx, " \t"))) {
		varsetc("_", cur);
		p = varexpand(args[1]);
		n = strlen(p);
		out = xrealloc(out, len+n+2);
		memcpy(out+len, p, n);
		len += n;
		out[len++] = ' ';
		free(p);
		varunset("_");
	}

	free(arr);
	if (out)
		out[len] = '\0';
	return out;
}

static char *stripext(vec(char*) args) {
	char *s, *out;
	s = varexpand(args[0]);
	out = xstrndup(s, strrchr(s, '.') - s);
	free(s);
	return out;
}

static char *glob_(vec(char*) args) {
	char *pattern, *out;
	size_t outlen, pathlen;
	vec(char*) paths = NULL;

	pattern = varexpand(args[0]);
	glob(pattern, &paths);

	out = NULL;
	outlen = 0;
	for (size_t i = 0; i < vec_size(paths); i++) {
		pathlen = strlen(paths[i]);
		out = xrealloc(out, outlen + pathlen + 1);
		memcpy(out+outlen, paths[i], pathlen);
		outlen += pathlen;
		out[outlen++] = ' ';
		free(paths[i]);
	}
	if (out)
		out[outlen-1] = 0;

	free(pattern);
	vec_free(paths);
	return out;
}
