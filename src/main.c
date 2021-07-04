#include <stdio.h>
#include <stdlib.h>
#include "dale.h"

const char *tasktypes[] = {0, "exe", "lib", "dll"};

struct task *tasks;
size_t ntasks;

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	char *p, *p2;
	int skip;

	parse("build.dale");

	if (tasks) {
		hostmkdir("build");
		for (size_t i = 0; i < ntasks; i++) {
			skip = asprintf(&p, "build/%s_obj", tasks[i].name);
			hostmkdir(p);
			free(p);
			for (size_t j = 0; j < tasks[i].nsrcs; j++) {
				asprintf(&p, "build/%s_obj/%s", tasks[i].name, tasks[i].srcs[j]);
				for (p2 = p + skip + 1; *p2; p2++) {
					if (*p2 == '/') {
						*p2 = 0;
						hostmkdir(p);
						*p2 = '/';
					}
				}
				free(p);
			}
		}
	}
}
