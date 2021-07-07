#include <stdio.h>
#include <stdlib.h>
#include "dale.h"

const char *tasktypes[] = {0, "exe", "lib", "dll"};

static const char *builtin[] = {
#ifdef _WIN32
"toolchain(msvc)",
"	find: cl link",
"	objext: .obj",
"	libprefix: /DEFAULTLIB:",
"	compile: \"$CL\" /nologo /c /Fo:$out $in",
"	linkexe: \"$LINK\" /NOLOGO $lib /OUT:$out.exe $in",
#endif
"toolchain(gcc)",
"	find: gcc",
"	objext: .o",
"	libprefix: -l",
"	compile: \"$GCC\" -c -o $out $in",
"	linkexe: \"$GCC\" -o $out $in $lib",
};

struct task *tasks;
size_t ntasks;
struct tc *tcs;
size_t ntcs;

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	char *p, *p2, *p3;
	struct tc *tc = NULL;
	int skip;

	hostinit();
	hostsetvars();
	parsea(builtin, LEN(builtin));
	parsef("build.dale");

	for (size_t i = 0; i < ntcs; i++) {
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
		}
	}

	hostquit();
}
