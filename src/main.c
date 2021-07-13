#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "dale.h"

const char *tasktypes[] = {0, "exe", "lib", "dll"};

static const char *builtin[] = {
#ifdef _WIN32
"toolchain(msvc)",
"	find: cl link lib",
"	objext: .obj",
"	libprefix: /DEFAULTLIB:",
"	compile: \"$CL\" /M$[!msvc_staticcrt D]$[msvc_staticcrt T]$[dll  /LD]$[debug d /Zi /Fd:build/$task] $[optfast /O2] $[optsize /O1] /nologo /c /Fo:$out $in",
"	linkexe: \"$LINK\" $[debug /DEBUG] /NOLOGO $lib /OUT:$out $in",
"	linklib: \"$LIB\" /NOLOGO /OUT:$out $in",
"	linkdll: \"$LINK\" $[debug /DEBUG] /NOLOGO /DLL $lib /OUT:$out $in",
#endif
"toolchain(gcc)",
"	find: gcc ar",
"	objext: .o",
"	libprefix: -l",
"	compile: \"$GCC\" $[debug -g] $[optfast -O3] $[optsize -Os] $[lib -fPIC] $[dll -fPIC] -c -o $out $in",
"	linkexe: \"$GCC\" -o $out $in $lib",
"	linklib: \"$AR\" -rc $out $in",
"	linkdll: \"$GCC\" -shared -o $out $in $lib",
};

struct task *tasks;
size_t ntasks;
struct tc *tcs;
size_t ntcs;

int main(int argc, char *argv[]) {
	char *p, *p2, *p3, *p4;
	char **arr;
	struct tc *tc = NULL;
	int skip;
	size_t sz;
	size_t taskn = 1;
	char **want = NULL;
	size_t nwant = 0;
	char *fflag = "build.dale";
	char *lflag = "local.dale";
	int pflag = 0;
	char *bdir, *tcname;
	bool verbose;
	char *exeext, *libext, *dllext;

	hostinit();

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 'h':
				case '?':
					printf(
						"usage: %s [options] [var=value]... [task]...\n"
						"\n"
						"options:\n"
						"  -h         Show this help\n"
						"  -f <file>  Set buildscript name (default: build.dale)\n"
						"  -l <file>  Set localscript name (default: local.dale)\n"
						"  -p         Process following var=value args after localscript\n"
						"  -v         Show executed commands\n"
					, argv[0]);
					return 0;
				case 'f':
					fflag = argv[++i];
					break;
				case 'l':
					lflag = argv[++i];
					break;
				case 'p':
					pflag = i+1;
					break;
				default:
					err("Unknown option '%s'", argv[i]);
			}
		} else if (strchr(argv[i], '=')) {
			if (!pflag) {
				sz = strcspn(argv[i], "=");
				p = xstrndup(argv[i], sz);
				p2 = xstrdup(argv[i] + sz + 1);
				varset(p, p2);
			}
		} else {
			want = xrealloc(want, sizeof(*want) * ++nwant);
			want[nwant-1] = argv[i];
		}
	}

	parsef(lflag, false);

	if (pflag) {
		for (int i = pflag; i < argc; i++) {
			if (strchr(argv[i], '=')) {
				sz = strcspn(argv[i], "=");
				p = xstrndup(argv[i], sz);
				p2 = xstrdup(argv[i] +  sz + 1);
				varset(p, p2);
			}
		}
	}

	if (*varget("nodefvar") != '1')
		hostsetvars();
	if (*varget("nodeftc") != '1')
		parsea(builtin, LEN(builtin));

	bdir = vargetnull("bdir");
	if (!bdir)
		bdir = "build";
	tcname = vargetnull("tcname");
	verbose = vargetnull("verbose");

	parsef(fflag, true);

	for (size_t i = ntcs; i;) {
		i--;
		if (tcname && strcmp(tcs[i].name, tcname))
			continue;
		if (!tcs[i].find || !tcs[i].objext || !tcs[i].libprefix || !tcs[i].compile || !tcs[i].linkexe) {
			fprintf(stderr, "Warning: Skipping underspecified toolchain '%s'\n", tcs[i].name);
			continue;
		}
		for (size_t j = 0; j < tcs[i].nfind; j++) {
			p2 = upperstr(tcs[i].find[j]);
			if (vargetnull(p2)) {
				free(p2);
				continue;
			}
			p = hostfind(tcs[i].find[j]);
			if (p) {
				varset(p2, p);
				if (j+1 == tcs[i].nfind)
					tc = &tcs[i];
			} else {
				free(p2);
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
		if (!tcname)
			fputs("Error: No valid toolchain found (tried:", stderr);
		else
			fprintf(stderr, "Error: Unknown toolchain '%s' (known:", tcname);
		for (size_t i = 0; i < ntcs; i++)
			fprintf(stderr, " %s", tcs[i].name);
		fputs(")\n", stderr);
		return 1;
	}
	printf("Using toolchain '%s'\n", tc->name);

	exeext = varget("exeext");
	libext = varget("libext");
	dllext = varget("dllext");

	for (size_t i = 0; i < nwant; i++) {
		for (size_t j = 0; j < ntasks; j++) {
			if (!strcmp(tasks[j].name, want[i])) {
				tasks[j].build = true;
				goto wantfound;
			}
		}
		err("Unknown task '%s'", want[i]);
wantfound:;
	}

	if (tasks) {
		hostmkdir(bdir);
		for (size_t i = 0; i < ntasks; i++) {
			if (nwant && !tasks[i].build)
				continue;

			printf("[%zu/%zu] %s\n", taskn++, nwant ? nwant : ntasks, tasks[i].name);
			varsetd("task", tasks[i].name);
			varsetd("exe", tasks[i].type == EXE ? "1" : "0");
			varsetd("lib", tasks[i].type == LIB ? "1" : "0");
			varsetd("dll", tasks[i].type == DLL ? "1" : "0");

			arr = tasks[i].srcs;
			sz = tasks[i].nsrcs;
			tasks[i].srcs = NULL;
			tasks[i].nsrcs = 0;
			for (size_t j = 0; j < sz; j++) {
				glob(arr[j], &tasks[i].srcs, &tasks[i].nsrcs);
				free(arr[j]);
			}
			free(arr);

			skip = asprintf(&p, "%s/%s_obj", bdir, tasks[i].name);
			hostmkdir(p);
			free(p);
			for (size_t j = 0; j < tasks[i].nsrcs; j++) {
				asprintf(&p, "%s/%s_obj/%s", bdir, tasks[i].name, tasks[i].srcs[j]);
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
				asprintf(&p, "%s/%s_obj/%s%s", bdir, tasks[i].name, tasks[i].srcs[j], tc->objext);
				asprintf(&p3, "%s %s", p2, p);
				free(p2);
				p2 = p3;
				if (hostfnewer(tasks[i].srcs[j], p)) {
					tasks[i].link = true;
					if (!verbose)
						printf("=> Compiling %s\n", tasks[i].srcs[j]);
					varsetd("in", tasks[i].srcs[j]);
					varsetp("out", p);
					p = varexpand(tc->compile);
					if (verbose)
						puts(p);
					system(p);
					varunset("in");
					varunset("out");
				}
				free(p);
			}

			switch (tasks[i].type) {
				case EXE:
					p3 = tc->linkexe;
					p4 = exeext;
					break;
				case LIB:
					p3 = tc->linklib;
					p4 = libext;
					break;
				case DLL:
					p3 = tc->linkdll;
					p4 = dllext;
					break;
			}
			asprintf(&p, "%s/%s%s", bdir, tasks[i].name, p4);
			if (tasks[i].link || !hostfexists(p)) {
				if (!verbose)
					printf("=> Linking %s\n", p);
				varsetp("in", p2);
				varsetp("out", p);
				if (tasks[i].type != LIB) {
					p = xstrdup("");
					for (size_t j = 0; j < tasks[i].nlibs; j++) {
						asprintf(&p2, "%s %s%s", p, tc->libprefix, tasks[i].libs[j]);
						free(p);
						p = p2;
					}
					varsetp("lib", p);
				}
				p = varexpand(p3);
				if (verbose)
					puts(p);
				system(p);
				varunset("in");
				varunset("out");
				if (tasks[i].type != LIB)
					varunset("lib");
			} else {
				puts("(nothing to do)");
				free(p2);
			}
			free(p);

			varunset("task");
		}
	}

	hostquit();
}
