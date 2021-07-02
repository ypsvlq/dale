#include <stdio.h>
#include <string.h>
#include "dale.h"

#define LINE_MAX 2048

void parse(FILE *f) {
	static char buf[LINE_MAX], s1[LINE_MAX], s2[LINE_MAX];
	char *p;
	size_t line;
	struct task *task;
	enum {
		NONE,
		TASK,
	} state;

	state = NONE;
	line = 0;
	while (fgets(buf, LINE_MAX, f)) {
		line++;
		p = buf + strspn(buf, " \t");
		if (*p == '\n' || *p == '#')
			continue;
		if (buf == p) {
			if (sscanf(p, "%[^( ] ( %[^)]", s1, s2) == 2) {
				state = TASK;
				tasks = xrealloc(tasks, sizeof(*tasks) * ++ntasks);
				task = tasks + ntasks - 1;
				*task = (struct task){0};
				task->name = xstrdup(s2);
				for (size_t i = 1; i < LEN(tasktypes); i++)
					if (!strcmp(s1, tasktypes[i]))
						task->type = i;
				if (!task->type)
					err("Invalid task type '%s'", s1);
				continue;
			}
		} else {
			if (sscanf(p, "%[^: ] : %[^\n]", s1, s2) == 2) {
				if (state == TASK) {
					if (!strcmp(s1, "src")) {
						p = strtok(s2, " \t");
						while (p) {
							task->srcs = xrealloc(task->srcs, sizeof(*task->srcs) * ++task->nsrcs);
							task->srcs[task->nsrcs-1] = xstrdup(p);
							p = strtok(NULL, " \t");
						}
						continue;
					}
					err("Invalid task variable '%s'", s1);
				}
			}
		}
		err("Syntax error");
	}
}
