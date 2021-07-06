#include <stdio.h>
#include <stdlib.h>
#include "dale.h"

const char *tasktypes[] = {0, "exe", "lib", "dll"};

struct task *tasks;
size_t ntasks;

static struct tc {
	char *name;
	char *objext;
	char **find;
	char *compile, *linkexe;
} tcs[] = {
#ifdef _WIN32
	{
		"msvc", ".obj", (char*[]){"cl", "link", 0},
		"\"$CL\" /nologo /c /Fo:$out $in",
		"\"$LINK\" /NOLOGO /OUT:$out $in",
	},
#endif
	{
		"gcc", ".o", (char*[]){"gcc", 0},
		"\"$GCC\" -c -o $out $in",
		"\"$GCC\" -o $out $in",
	},
};

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	char *p, *p2, *p3;
	struct tc *tc = NULL;
	int skip;

	hostinit();
	parse("build.dale");

	for (size_t i = 0; i < LEN(tcs); i++) {
		for (size_t j = 0; tcs[i].find[j]; j++) {
			p = hostfind(tcs[i].find[j]);
			if (p) {
				varset(upperstr(tcs[i].find[j]), p);
				if (!tcs[i].find[j+1])
					tc = &tcs[i];
			} else {
				for (size_t k = j; k;) {
					k--;
					p = upperstr(tcs[i].find[k]);
					varunset(p);
					free(p);
				}
				break;
			}
		}
		if (tc)
			break;
	}
	if (!tc) {
		fputs("Error: No valid toolchain found (tried:", stderr);
		for (size_t i = 0; i < LEN(tcs); i++)
			fprintf(stderr, " %s", tcs[i].name);
		fputs(")\n", stderr);
		return 1;
	}

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

			p2 = xstrdup("");
			for (size_t j = 0; j < tasks[i].nsrcs; j++) {
				varsetd("in", tasks[i].srcs[j]);
				asprintf(&p, "build/%s_obj/%s%s", tasks[i].name, tasks[i].srcs[j], tc->objext);
				varsetp("out", p);
				asprintf(&p3, "%s %s", p2, p);
				free(p2);
				p2 = p3;
				p = varexpand(tc->compile);
				system(p);
				free(p);
				varunset("in");
				varunset("out");
			}
			varsetp("in", p2);
			asprintf(&p, "build/%s", tasks[i].name);
			varsetp("out", p);
			p = varexpand(tc->linkexe);
			system(p);
			free(p);
			varunset("in");
			varunset("out");
		}
	}

	hostquit();
}
