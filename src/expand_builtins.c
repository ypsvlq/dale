#include <stdlib.h>
#include <string.h>
#include "dale.h"

static char *map(char **);
static char *stripext(char **);
static char *glob_(char **);

const struct ebuiltin ebuiltins[] = {
	{"map", map, 2, true},
	{"stripext", stripext, 1, false},
	{"glob", glob_, 1, false},
	{0}
};

static char *map(char **args) {
	char *out = NULL;
	size_t len = 0;
	char *arr, *cur, *p, *ctx;
	size_t n;

	arr = args[0];
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

	if (out)
		out[len] = '\0';
	return out;
}

static char *stripext(char **args) {
	return xstrndup(args[0], strrchr(args[0], '.') - args[0]);
}

static char *glob_(char **args) {
	char *out;
	size_t outlen, pathlen;
	vec(char*) paths = NULL;

	glob(args[0], &paths);
	out = NULL;
	outlen = 0;
	for (size_t i = 0; i < vec_len(paths); i++) {
		pathlen = strlen(paths[i]);
		out = xrealloc(out, outlen + pathlen + 1);
		memcpy(out+outlen, paths[i], pathlen);
		outlen += pathlen;
		out[outlen++] = ' ';
		free(paths[i]);
	}

	if (out)
		out[outlen-1] = 0;
	vec_free(paths);
	return out;
}
