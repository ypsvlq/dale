#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
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
	static const char *tasktypes[] = {0, "exe", "lib", "dll"};
	static const char *decltypes[] = {0, "task", "toolchain"};
	static char s1[LINE_MAX], s2[LINE_MAX];
	const char *buf, *p;
	char *p2, *p3;
	enum {NONE, TASK, TOOLCHAIN} state;
	struct task *task;
	struct tc *tc;
	struct vars {
		struct var {
			char *name;
			char **out;
		} a[10];
	} vars;
	struct lists {
		struct list {
			char *name;
			char ***out;
			size_t *outsz;
		} a[6];
	} lists;

	state = NONE;
	while ((buf = read(data))) {
		line++;
		p = buf + strspn(buf, " \t");
		if (*p == '\n' || *p == '#')
			continue;
		if (buf == p) {
			if (sscanf(p, "%[^= ] = %[^\n]", s1, s2) == 2) {
				state = NONE;
				varsetp(s1, varexpand(s2));
				continue;
			} else if (sscanf(p, "%[^+ ] += %[^\n]", s1, s2) == 2) {
				state = NONE;
				p2 = varexpand(s2);
				asprintf(&p3, "%s %s", varget(s1), p2);
				free(p2);
				varupdate(s1, p3);
				continue;
			} else if (sscanf(p, "%[^( ] ( %[^)]", s1, s2) == 2) {
				if (strlen(s2) != strspn(s2, valid))
					err("Invalid name '%s'", s2);
				if (!strcmp(s1, "toolchain")) {
					state = TOOLCHAIN;
					tcs = xrealloc(tcs, sizeof(*tcs) * ++ntcs);
					tc = &tcs[ntcs-1];
					*tc = (struct tc){0};
					tc->name = xstrdup(s2);
					vars = (struct vars){{
						{"objext", &tc->objext},
						{"libext", &tc->libext},
						{"libfmt", &tc->libfmt},
						{"incfmt", &tc->incfmt},
						{"deffmt", &tc->deffmt},
						{"compile", &tc->compile},
						{"linkexe", &tc->linkexe},
						{"linklib", &tc->linklib},
						{"linkdll", &tc->linkdll},
						{0}
					}};
					lists = (struct lists){{
						{"find", &tc->find, &tc->nfind},
						{0}
					}};
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
					lists = (struct lists){{
						{"src", &task->srcs, &task->nsrcs},
						{"lib", &task->libs, &task->nlibs},
						{"inc", &task->incs, &task->nincs},
						{"def", &task->defs, &task->ndefs},
						{"req", &task->reqs, &task->nreqs},
						{0}
					}};
					continue;
				}
			}
		} else {
			if (state != NONE && sscanf(p, "%[^: ] : %[^\n]", s1, s2) == 2) {
				if (state == TASK || state == TOOLCHAIN) {
					for (struct list *l = lists.a; l->name; l++) {
						if (!strcmp(s1, l->name)) {
							loadarr(l->out, l->outsz, s2);
							goto assigned;
						}
					}
				}
				if (state == TOOLCHAIN) {
					for (struct var *v = vars.a; v->name; v++) {
						if (!strcmp(s1, v->name)) {
							*(v->out) = xstrdup(s2);
							goto assigned;
						}
					}
				}
				err("Unknown %s variable '%s'", decltypes[state], s1);
			}
		}
		err("Syntax error");
assigned:;
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

void parsef(const char *path, bool required) {
	FILE *f;
	char *path2;
	asprintf(&path2, "%s.dale", path);
	f = fopen(path2, "r");
	if (!f) {
		f = fopen(path, "r");
		if (!f) {
			if (required)
				err("Could not open '%s'", path);
			free(path2);
			return;
		}
		fname = path;
	} else {
		fname = path2;
	}
	parse(readf, f);
	if (ferror(f))
		err("Failed reading '%s'", fname);
	fclose(f);
	free(path2);
}
