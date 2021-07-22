#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "dale.h"

static const char *builtin[] = {
#ifdef _WIN32
"toolchain(msvc)",
"	lang: c",
"	find: cl link lib",
"	objext: .obj",
"	libext: .lib",
"	libpfx: /DEFAULTLIB:",
"	incpfx: /I",
"	defpfx: /D",
"	compile: \"$CL\" $CFLAGS /M$[!msvc_staticcrt D]$[msvc_staticcrt T]$[dll  /LD]$[debug d /Zi /Fd:build/$task] $[optfast /O2] $[optsize /O1] /nologo /c /Fo:$out $in",
"	linkexe: \"$LINK\" $LEFLAGS $[debug /DEBUG] /NOLOGO $lib /OUT:$out $in $LIBS",
"	linklib: \"$LIB\" $LLFLAGS /NOLOGO /OUT:$out $in",
"	linkdll: \"$LINK\" $LDFLAGS $[debug /DEBUG] /NOLOGO /DLL $lib /OUT:$out $in $LIBS",
#endif
"toolchain(clang)",
"	lang: c",
"	find: clang ar",
"	objext: .o",
"	libext: .a",
"	libpfx: -l",
"	incpfx: -I",
"	defpfx: -D",
"	compile: \"$CLANG\" $CFLAGS $[debug -g] $[optfast -O3] $[optsize -Oz] $[lib -fPIC] $[dll -fPIC] -c -o $out $in",
"	linkexe: \"$CLANG\" $LEFLAGS -o $out $in $LIBS",
"	linklib: \"$AR\" -rc $LLFLAGS $out $in",
"	linkdll: \"$CLANG\" $LDFLAGS -shared -o $out $in $LIBS",
"toolchain(gcc)",
"	lang: c",
"	find: gcc ar",
"	objext: .o",
"	libext: .a",
"	libpfx: -l",
"	incpfx: -I",
"	defpfx: -D",
"	compile: \"$GCC\" $CFLAGS $[debug -g] $[optfast -O3] $[optsize -Os] $[lib -fPIC] $[dll -fPIC] -c -o $out $in",
"	linkexe: \"$GCC\" $LEFLAGS -o $out $in $LIBS",
"	linklib: \"$AR\" -rc $LLFLAGS $out $in",
"	linkdll: \"$GCC\" $LDFLAGS -shared -o $out $in $LIBS",
"toolchain(clang++)",
"	lang: c++",
"	find: clang++ ar",
"	objext: .o",
"	libext: .a",
"	libpfx: -l",
"	incpfx: -I",
"	defpfx: -D",
"	compile: \"$CLANG++\" $CFLAGS $[debug -g] $[optfast -O3] $[optsize -Oz] $[lib -fPIC] $[dll -fPIC] -c -o $out $in",
"	linkexe: \"$CLANG++\" $LEFLAGS -o $out $in $LIBS",
"	linklib: \"$AR\" -rc $LLFLAGS $out $in",
"	linkdll: \"$CLANG++\" $LDFLAGS -shared -o $out $in $LIBS",
"toolchain(g++)",
"	lang: c++",
"	find: g++ ar",
"	objext: .o",
"	libext: .a",
"	libpfx: -l",
"	incpfx: -I",
"	defpfx: -D",
"	compile: \"$G++\" $CFLAGS $[debug -g] $[optfast -O3] $[optsize -Os] $[lib -fPIC] $[dll -fPIC] -c -o $out $in",
"	linkexe: \"$G++\" $LEFLAGS -o $out $in $LIBS",
"	linklib: \"$AR\" -rc $LLFLAGS $out $in",
"	linkdll: \"$G++\" $LDFLAGS -shared -o $out $in $LIBS",
};

struct task *tasks;
size_t ntasks;
struct tc *tcs;
size_t ntcs;

static char *upperstr(const char *s) {
	char *out = xmalloc(strlen(s) + 1);
	for (size_t i = 0; s[i]; i++)
		out[i] = toupper(s[i]);
	out[strlen(s)] = 0;
	return out;
}

static void varpfxarr(const char *var, const char *pfx, char **arr, size_t len) {
	char *out, *tmp;
	if (!len)
		return;
	asprintf(&out, "%s%s ", pfx, arr[0]);
	for (size_t i = 1; i < len; i++) {
		asprintf(&tmp, "%s%s%s ", out, pfx, arr[i]);
		free(out);
		out = tmp;
	}
	varsetp(var, out);
}

static void taskvarset(const char *var, const char *name) {
	char *p;
	asprintf(&p, "$%s $%s_%s", var, name, var);
	varsetp(var, varexpand(p));
	free(p);
}

int main(int argc, char *argv[]) {
	char *p, *p2, *p3, *p4;
	char **arr;
	struct tc *tc = NULL;
	int skip;
	size_t sz;
	size_t taskn = 1;
	char **want = NULL;
	size_t nwant = 0;
	char **lflag = NULL;
	size_t nlflag = 0;
	int pflag = 0;
	char *bscript, *bdir, *tcname, *dalereq, *lang;
	bool verbose;
	char *exeext, *dllext;

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
						"  -l <file>  Load localscript from path, can be repeated to specify multiple\n"
						"  -p         Process following var=value args after localscript(s)\n"
					, argv[0]);
					return 0;
				case 'l':
					lflag = xrealloc(lflag, sizeof(*lflag) * ++nlflag);
					lflag[nlflag-1] = argv[++i];
					break;
				case 'p':
					if (pflag)
						fprintf(stderr, "Warning: '-p' option used multiple times\n");
					else
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

	if (!lflag) {
		parsef("local.dale", false);
	} else {
		for (size_t i = 0; i < nlflag; i++)
			parsef(lflag[i], true);
		free(lflag);
	}

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

	bscript = vargetnull("bscript");
	if (!bscript)
		bscript = "build.dale";
	bdir = vargetnull("bdir");
	if (!bdir)
		bdir = "build";
	tcname = vargetnull("tcname");
	verbose = vargetnull("verbose");
	dalereq = NULL;
	if (!vargetnull("nodalereq"))
		if (!(dalereq = vargetnull("DALEREQ")))
			dalereq = hostfind("dalereq");

	parsef(bscript, true);
	lang = vargetnull("lang");
	if (!lang)
		lang = "c";

	for (size_t i = 0; i < ntcs; i++) {
		if (tcname && strcmp(tcs[i].name, tcname))
			continue;
		if (!tcs[i].find || !tcs[i].objext || !tcs[i].libext || !tcs[i].libpfx || !tcs[i].incpfx || !tcs[i].defpfx || !tcs[i].compile || !tcs[i].linkexe || !tcs[i].linklib || !tcs[i].linkdll) {
			fprintf(stderr, "Warning: Skipping underspecified toolchain '%s'\n", tcs[i].name);
			continue;
		}
		if (strcmp(tcs[i].lang, lang))
			continue;
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
			if (!strcmp(tcs[i].lang, lang))
				fprintf(stderr, " %s", tcs[i].name);
		fputs(")\n", stderr);
		return 1;
	}
	printf("Using toolchain '%s'\n", tc->name);

	exeext = varget("exeext");
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
		varsetp("LEFLAGS", varexpand("$LFLAGS $LEFLAGS"));
		varsetp("LDFLAGS", varexpand("$LFLAGS $LDFLAGS"));

		for (size_t i = 0; i < ntasks; i++) {
			if (nwant && !tasks[i].build)
				continue;

			printf("[%zu/%zu] %s\n", taskn++, nwant ? nwant : ntasks, tasks[i].name);
			varsetd("task", tasks[i].name);
			varsetd("exe", tasks[i].type == EXE ? "1" : "0");
			varsetd("lib", tasks[i].type == LIB ? "1" : "0");
			varsetd("dll", tasks[i].type == DLL ? "1" : "0");
			varpfxarr("CFLAGS", tc->incpfx, tasks[i].incs, tasks[i].nincs);
			varpfxarr("CFLAGS", tc->defpfx, tasks[i].defs, tasks[i].ndefs);
			taskvarset("CFLAGS", tasks[i].name);
			taskvarset("LIBS", tasks[i].name);
			taskvarset("LEFLAGS", tasks[i].name);
			taskvarset("LLFLAGS", tasks[i].name);
			taskvarset("LDFLAGS", tasks[i].name);

			for (size_t j = 0; j < tasks[i].nreqs; j++) {
				asprintf(&p, "HAVE_%s", tasks[i].reqs[j]);
				if (!vargetnull(p)) {
					free(p);
					if (dalereq) {
						asprintf(&p, "%s cflags %s", dalereq, tasks[i].reqs[j]);
						p2 = hostexecout(p);
						free(p);
						if (p2) {
							asprintf(&p3, "%s %s", varget("CFLAGS"), p2);
							varsetp("CFLAGS", p3);
							free(p2);
							asprintf(&p, "%s libs %s", dalereq, tasks[i].reqs[j]);
							p2 = hostexecout(p);
							free(p);
							if (p2) {
								asprintf(&p3, "%s %s", varget("LIBS"), p2);
								varsetp("LIBS", p3);
								free(p2);
								continue;
							}
						}
					}
					err("Required library '%s' not defined", tasks[i].reqs[j]);
				} else {
					taskvarset("CFLAGS", tasks[i].reqs[j]);
					taskvarset("LIBS", tasks[i].reqs[j]);
				}
				free(p);
			}

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
					if (system(p))
						err("Compilation failed");
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
					p4 = tc->libext;
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
				if (tasks[i].type != LIB)
					varpfxarr("LIBS", tc->libpfx, tasks[i].libs, tasks[i].nlibs);
				p = varexpand(p3);
				if (verbose)
					puts(p);
				if (system(p))
					err("Linking failed");
				varunset("in");
				varunset("out");
				if (tasks[i].type != LIB)
					varunset("lib");
			} else {
				puts("(nothing to do)");
				free(p2);
			}
			free(p);

			for (size_t j = 0; j < tasks[i].nreqs; j++) {
				varunset("CFLAGS");
				varunset("LIBS");
			}

			varunset("task");
			varunset("exe");
			varunset("lib");
			varunset("dll");
			varunset("inc");
			varunset("def");
			varunset("CFLAGS");
			varunset("LIBS");
			varunset("LEFLAGS");
			varunset("LLFLAGS");
			varunset("LDFLAGS");
		}
	}

	hostquit();
}
