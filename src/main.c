#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "dale.h"

const char *tasktypes[] = {0, "exe", "lib", "dll"};

static const char *builtin[] = {
#ifdef _WIN32
"toolchain(msvc)",
"	find: cl link",
"	objext: .obj",
"	libprefix: /DEFAULTLIB:",
"	compile: \"$CL\" /MD$[debug d /Zi /Fd:build/$task] $[optfast /O2] $[optsize /O1] /nologo /c /Fo:$out $in",
"	linkexe: \"$LINK\" $[debug /DEBUG] /NOLOGO $lib /OUT:$out.exe $in",
#endif
"toolchain(gcc)",
"	find: gcc",
"	objext: .o",
"	libprefix: -l",
"	compile: \"$GCC\" $[debug -g] $[optfast -O3] $[optsize -Os] -c -o $out $in",
"	linkexe: \"$GCC\" -o $out $in $lib",
};

struct task *tasks;
size_t ntasks;
struct tc *tcs;
size_t ntcs;

int main(int argc, char *argv[]) {
	char *p, *p2, *p3;
	struct tc *tc = NULL;
	int skip;
	size_t taskn = 1;

	hostinit();
	hostsetvars();

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			for (int j = 1; argv[i][j]; j++) {
				switch (argv[i][j]) {
					case 'h':
					case '?':
						printf(
							"usage: %s [options]\n"
							"\n"
							"options:\n"
							"  -h    Show this help\n"
							"  -g    Include debugging information\n"
							"  -O    Optimise for speed\n"
							"  -S    Optimise for size\n"
						, argv[0]);
						return 0;
					case 'g':
						varsetd("debug", "1");
						break;
					case 'O':
						varsetd("optfast", "1");
						break;
					case 'S':
						varsetd("optsize", "1");
						break;
					default:
						err("Unknown option '%s'", argv[i]);
				}
			}
		}
	}

	parsea(builtin, LEN(builtin));
	parsef("local.dale", false);
	parsef("build.dale", true);

	for (size_t i = 0; i < ntcs; i++) {
		if (!tcs[i].find || !tcs[i].objext || !tcs[i].libprefix || !tcs[i].compile || !tcs[i].linkexe) {
			fprintf(stderr, "Warning: Skipping underspecified toolchain '%s'\n", tcs[i].name);
			continue;
		}
		for (size_t j = 0; j < tcs[i].nfind; j++) {
			p = hostfind(tcs[i].find[j]);
			if (p) {
				varset(upperstr(tcs[i].find[j]), p);
				if (j+1 == tcs[i].nfind)
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
		for (size_t i = 0; i < ntcs; i++)
			fprintf(stderr, " %s", tcs[i].name);
		fputs(")\n", stderr);
		return 1;
	}
	printf("Using toolchain '%s'\n", tc->name);

	if (tasks) {
		hostmkdir("build");
		for (size_t i = 0; i < ntasks; i++) {
			printf("[%zu/%zu] %s\n", taskn++, ntasks, tasks[i].name);
			varsetd("task", tasks[i].name);

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
				printf("=> Compiling %s\n", tasks[i].srcs[j]);
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
			printf("=> Linking %s\n", p);
			varsetp("out", p);
			p = xstrdup("");
			for (size_t j = 0; j < tasks[i].nlibs; j++) {
				asprintf(&p2, "%s %s%s", p, tc->libprefix, tasks[i].libs[j]);
				free(p);
				p = p2;
			}
			varsetp("lib", p);
			p = varexpand(tc->linkexe);
			system(p);
			free(p);
			varunset("in");
			varunset("out");
			varunset("lib");

			varunset("task");
		}
	}

	hostquit();
}
