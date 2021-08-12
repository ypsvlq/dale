#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "dale.h"

#define LINE_MAX 2048

const char *fname;
size_t line;

static void parse(const char *(*read)(void *data), void *data) {
	static char s1[LINE_MAX], s2[LINE_MAX];
	const char *buf, *p;
	char *p2, *cur, *ctx;
	size_t n;
	char **args;
	enum {NONE, TASK, BUILD, RULE} state;
	struct task *task = NULL;
	struct build *build = NULL;
	struct rule *rule = NULL;

	state = NONE;
	while ((buf = read(data))) {
		line++;
		p = buf + strspn(buf, " \t");
		if (*p == '\n' || *p == '#') {
			if (state == BUILD)
				vec_push(build->steps, "\n");
			continue;
		}
		if (buf == p) {
			if (*p == '@') {
				p++;
				for (size_t i = 0; pbuiltins[i].name; i++) {
					n = strlen(pbuiltins[i].name);
					if (!strncmp(pbuiltins[i].name, p, n) && isspace(p[n])) {
						p += n+1;
						args = xmalloc(sizeof(*args) * pbuiltins[i].nargs);
						p2 = xstrdup(p);
						cur = strpbrk(p2, "\r\n");
						if (cur)
							*cur = '\0';
						ctx = p2;
						n = 0;
						if (*p2) {
							while ((cur = rstrtok(&ctx, " \t"))) {
								args[n++] = cur;
								if (n == pbuiltins[i].nargs)
									break;
							}
							while (rstrtok(&ctx, " \t"))
								n++;
						}
						if (n != pbuiltins[i].nargs)
							err("Builtin '%s' takes %zu arguments but got %zu", pbuiltins[i].name, pbuiltins[i].nargs, n);
						pbuiltins[i].fn(args);
						free(p2);
						free(args);
						goto assigned;
					}
				}
				err("Unknown builtin '%s'", xstrndup(p, strcspn(p, " \t")));
			} else if (sscanf(p, "%[^= ] = %[^\n]", s1, s2) == 2) {
				state = NONE;
				varset(xstrdup(s1), varexpand(s2));
				continue;
			} else if (sscanf(p, "%[^+ ] += %[^\n]", s1, s2) == 2) {
				state = NONE;
				p2 = varexpand(s2);
				varappend(s1, p2);
				free(p2);
				continue;
			} else if (sscanf(p, "%[^? ] ?= %[^\n]", s1, s2) == 2) {
				state = NONE;
				if (!vargetnull(s1))
					varset(xstrdup(s1), varexpand(s2));
				continue;
			} else if (sscanf(p, "%[^( ] ( %[^)]", s1, s2) == 2) {
				if (!strcmp(s1, "build")) {
					state = BUILD;
					vec_push(builds, (struct build){.name = xstrdup(s2)});
					build = &builds[vec_len(builds)-1];
					build->fname = xstrdup(fname);
					build->line = line;
					continue;
				} else if (!strcmp(s1, "rule")) {
					state = RULE;
					p = strchr(s2, '.');
					if (!p)
						err("Invalid rule name '%s'", s2);
					n = p - s2;
					for (struct build *build = builds; build < vec_end(builds); build++) {
						if (!strncmp(build->name, s2, n) && strlen(build->name) == n) {
							vec_push(build->rules, (struct rule){.name = xstrdup(p+1)});
							rule = vec_end(build->rules) - 1;
							goto assigned;
						}
					}
					err("Unknown build '%s'", xstrndup(s2, n));
				} else {
					if (strlen(s2) != strspn(s2, valid))
						err("Invalid task name '%s'", s2);
					state = TASK;
					vec_push(tasks, (struct task){0});
					task = vec_end(tasks) - 1;
					task->name = xstrdup(s2);
					p = strchr(s1, '.');
					task->type = xstrdup(p ? p+1 : s1);
					for (struct build *build = builds; build < vec_end(builds); build++) {
						if ((!p && !strcmp(build->name, "c")) || (p && !strncmp(build->name, s1, p-s1))) {
							task->build = build;
							goto buildfound;
						}
					}
					err("Unknown build '%s'", p ? xstrndup(s1, p-s1) : "c");
buildfound:
					continue;
				}
			}
		} else {
			if (state == BUILD) {
				vec_push(build->steps, xstrdup(p));
				continue;
			} else if (state == RULE) {
				vec_push(rule->cmds, xstrdup(p));
				continue;
			} else if (state == TASK && sscanf(p, "%[^: ] : %[^\n]", s1, s2) == 2) {
				for (struct taskvar *var = task->vars; var < vec_end(task->vars); var++) {
					if (!strcmp(var->name, s1)) {
						p2 = varexpand(s2);
						asprintf(&cur, "%s %s", var->val, p2);
						free(var->val);
						free(p2);
						var->val = cur;
						continue;
					}
				}
				vec_push(task->vars, (struct taskvar){0});
				task->vars[vec_len(task->vars)-1] = (struct taskvar){
					.name = xstrdup(s1),
					.val = varexpand(s2),
				};
				continue;
			}
		}
		err("Syntax error");
assigned:;
	}

	line = 0;
}

struct reada {
	char **arr;
	size_t len;
};

static const char *reada(void *data) {
	struct reada *d = data;
	if (d->len--)
		return *(d->arr++);
	return NULL;
}

void parsea(vec(char*) vec, const char *name, size_t linestart) {
	fname = name;
	line = linestart;
	parse(reada, &(struct reada){vec, vec_len(vec)});
}

static const char *readf(void *data) {
	static char buf[LINE_MAX];
	FILE *f = data;
	return fgets(buf, LINE_MAX, f);
}

void parsef(const char *path, bool required) {
	FILE *f = NULL;
	char *path2 = NULL;
	size_t len = strlen(path);
	if (len < 5 || memcmp(path + len - 5, ".dale", 5)) {
		asprintf(&path2, "%s.dale", path);
		f = fopen(path2, "r");
		fname = path2;
	}
	if (!f) {
		f = fopen(path, "r");
		if (!f) {
			if (required)
				err("Could not open '%s'", path);
			free(path2);
			return;
		}
		fname = path;
	}
	parse(readf, f);
	if (ferror(f))
		err("Failed reading '%s'", fname);
	fclose(f);
	free(path2);
}
