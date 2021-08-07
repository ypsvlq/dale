#include <stdlib.h>
#include <string.h>
#include "dale.h"

static char *find(vec(char*));
static char *map(vec(char*));
static char *stripext(vec(char*));

const struct ebuiltin ebuiltins[] = {
	{"find", find, 1},
	{"map", map, 2},
	{"stripext", stripext, 1},
	{0}
};

static char *find(vec(char*) args) {
	return hostfind(args[0]);
}

static char *map(vec(char*) args) {
	char *out = NULL;
	size_t len = 0;
	char *arr, *cur, *p, *ctx;
	size_t n;

	if (vec_size(args) < 3)
		err("Nothing to map");

	arr = xstrdup(varget(args[0]));
	ctx = arr;
	while ((cur = rstrtok(&ctx, " \t"))) {
		varsetc(args[1], cur);
		p = varexpand(args[2]);
		n = strlen(p);
		out = xrealloc(out, len+n+2);
		memcpy(out+len, p, n);
		len += n;
		out[len++] = ' ';
		free(p);
		varunset(args[1]);
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
