#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dale.h"

#define LINE_MAX 2048

const char *fname;
size_t line;

static void loadarr(char ***out, size_t *outsz, char *val) {
	char *p, *p2;
	p2 = varexpand(val);
	p = strtok(p2, " \t");
	while (p) {
		*out = xrealloc(*out, sizeof(**out) * ++*outsz);
		(*out)[*outsz - 1] = xstrdup(p);
		p = strtok(NULL, " \t");
	}
	free(p2);
}

void parse(const char *(*read)(void *data), void *data) {
	static char s1[LINE_MAX], s2[LINE_MAX];
	const char *buf, *p;
	enum {NONE, TASK, TOOLCHAIN} state;
	struct task *task;
	struct tc *tc;

	state = NONE;
	while ((buf = read(data))) {
		line++;
		p = buf + strspn(buf, " \t");
		if (*p == '\n' || *p == '#')
			continue;
		if (buf == p) {
			if (sscanf(p, "%[^= ] = %[^\n]", s1, s2) == 2) {
				varsetp(s1, varexpand(s2));
				continue;
			} else if (sscanf(p, "%[^( ] ( %[^)]", s1, s2) == 2) {
				if (!strcmp(s1, "toolchain")) {
					state = TOOLCHAIN;
					tcs = xrealloc(tcs, sizeof(*tcs) * ++ntcs);
					tc = &tcs[ntcs-1];
					*tc = (struct tc){0};
					tc->name = xstrdup(s2);
					continue;
				} else {
					state = TASK;
					tasks = xrealloc(tasks, sizeof(*tasks) * ++ntasks);
					task = &tasks[ntasks-1];
					*task = (struct task){0};
					task->name = xstrdup(s2);
					for (size_t i = 1; i < LEN(tasktypes); i++)
						if (!strcmp(s1, tasktypes[i]))
							task->type = i;
					if (!task->type)
						err("Invalid task type '%s'", s1);
					continue;
				}
			}
		} else {
			if (sscanf(p, "%[^: ] : %[^\n]", s1, s2) == 2) {
				if (state == TASK) {
					if (!strcmp(s1, "src"))
						loadarr(&task->srcs, &task->nsrcs, s2);
					else if (!strcmp(s1, "lib"))
						loadarr(&task->libs, &task->nlibs, s2);
					else
						err("Invalid task variable '%s'", s1);
					continue;
				} else if (state == TOOLCHAIN) {
					if (!strcmp(s1, "find"))
						loadarr(&tc->find, &tc->nfind, s2);
					else if (!strcmp(s1, "objext"))
						tc->objext = xstrdup(s2);
					else if (!strcmp(s1, "libprefix"))
						tc->libprefix = xstrdup(s2);
					else if (!strcmp(s1, "compile"))
						tc->compile = xstrdup(s2);
					else if (!strcmp(s1, "linkexe"))
						tc->linkexe = xstrdup(s2);
					else
						err("Invalid toolchain variable '%s'", s1);
					continue;
				}
			}
		}
		err("Syntax error");
	}

	line = 0;
}

struct reada {
	const char **arr;
	size_t len;
};

static const char *reada(void *data) {
	struct reada *d = data;
	if (d->len--)
		return *(d->arr++);
	return NULL;
}

void parsea(const char *arr[], size_t len) {
	fname = "<builtin>";
	parse(reada, &(struct reada){arr, len});
}

static const char *readf(void *data) {
	static char buf[LINE_MAX];
	FILE *f = data;
	return fgets(buf, LINE_MAX, f);
}

void parsef(const char *path) {
	FILE *f = f = fopen(path, "r");
	if (!f)
		err("Could not open '%s'", path);
	fname = path;
	parse(readf, f);
	if (ferror(f))
		err("Failed reading '%s'", path);
	fclose(f);
}
