#include <stdio.h>
#include "dale.h"

const char *tasktypes[] = {0, "exe", "lib", "dll"};

struct task *tasks;
size_t ntasks;

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	FILE *f;

	f = fopen("build.dale", "r");
	if (!f)
		err("Could not open build.dale");
	parse(f);
	fclose(f);

	for (size_t i = 0; i < ntasks; i++) {
		printf("%s (%s) srcs =", tasks[i].name, tasktypes[tasks[i].type]);
		for (size_t j = 0; j < tasks[i].nsrcs; j++)
			printf(" %s", tasks[i].srcs[j]);
		putchar('\n');
	}
}
