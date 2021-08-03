#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <errno.h>
#include "dale.h"

vec(struct task) tasks;
vec(struct tc) tcs;

static vec(char*) optparse(int argc, char *argv[]);
static struct tc *loadtc(void);

static char *upperstr(const char *s);
static void varpfxarr(const char *var, const char *pfx, vec(char*) vec);
static void taskvarset(const char *var, const char *name);
static void globarr(vec(char*) *pvec);

int main(int argc, char *argv[]) {
	char *p, *p2, *p3, *p4;
	struct tc *tc = NULL;
	int skip;
	size_t sz;
	char **arr, **msgs;
	size_t taskn = 1;
	vec(char*) want;
	char *bscript, *bdir, *reqcflags, *reqlibs;
	size_t jobs = 0;
	uintmax_t um;
	bool verbose;
	char *exeext, *dllext;

	hostinit();

	want = optparse(argc, argv);

	if (*varget("_nodefvar") != '1')
		hostsetvars();

	bscript = vargetnull("_bscript");
	if (!bscript)
		bscript = "build.dale";
	bdir = vargetnull("_bdir");
	if (!bdir)
		bdir = "build";
	verbose = vargetnull("_verbose");
	reqcflags = vargetnull("_reqcflags");
	reqlibs = vargetnull("_reqlibs");
	p = vargetnull("_jobs");
	exeext = varget("_exeext");
	dllext = varget("_dllext");
	if (p) {
		errno = 0;
		um = strtoumax(p, NULL, 10);
		if (um > SIZE_MAX || errno == ERANGE)
			err("Overflow");
		jobs = um;
	}

	parsef(bscript, true);

	tc = loadtc();
	printf("Using toolchain '%s'\n", tc->name);

	for (size_t i = 0; i < vec_size(want); i++) {
		for (size_t j = 0; j < vec_size(tasks); j++) {
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

		for (struct task *task = tasks; task < vec_end(tasks); task++) {
			if (want && !task->build)
				continue;

			printf("[%zu/%zu] %s\n", taskn++, want ? vec_size(want) : vec_size(tasks), task->name);
			varsetd("task", task->name);
			varsetd("exe", task->type == EXE ? "1" : "0");
			varsetd("lib", task->type == LIB ? "1" : "0");
			varsetd("dll", task->type == DLL ? "1" : "0");
			taskvarset("CFLAGS", task->name);
			taskvarset("LIBS", task->name);
			taskvarset("LEFLAGS", task->name);
			taskvarset("LLFLAGS", task->name);
			taskvarset("LDFLAGS", task->name);
			globarr(&task->srcs);
			globarr(&task->incs);
			varpfxarr("CFLAGS", tc->incpfx, task->incs);
			varpfxarr("CFLAGS", tc->defpfx, task->defs);

			for (char **req = task->reqs; req < vec_end(task->reqs); req++) {
				asprintf(&p, "HAVE_%s", *req);
				if (!vargetnull(p)) {
					free(p);
					if (reqcflags && reqlibs) {
						asprintf(&p2, "%s_NAME", *req);
						p = vargetnull(p2);
						free(p2);
						if (!p)
							p = *req;
						varsetd("name", p);
						p = varexpand(reqcflags);
						p2 = hostexecout(p);
						free(p);
						if (p2) {
							varappend("CFLAGS", p2);
							free(p2);
							p = varexpand(reqlibs);
							p2 = hostexecout(p);
							free(p);
							if (p2) {
								varappend("LIBS", p2);
								free(p2);
								varunset("name");
								continue;
							}
						}
					}
					err("Required library '%s' not defined", *req);
				} else {
					taskvarset("CFLAGS", *req);
					taskvarset("LIBS", *req);
				}
				free(p);
			}

			skip = asprintf(&p, "%s/%s_obj", bdir, task->name);
			hostmkdir(p);
			free(p);
			for (char **src = task->srcs; src < vec_end(task->srcs); src++) {
				asprintf(&p, "%s/%s_obj/%s", bdir, task->name, *src);
				for (p2 = p + skip + 1; *p2; p2++) {
					if (*p2 == '/') {
						*p2 = 0;
						hostmkdir(p);
						*p2 = '/';
					}
				}
				free(p);
			}

			arr = NULL;
			msgs = NULL;
			sz = 0;

			p2 = xstrdup("");
			for (char **src = task->srcs; src < vec_end(task->srcs); src++) {
				asprintf(&p, "%s/%s_obj/%s%s", bdir, task->name, *src, tc->objext);
				asprintf(&p3, "%s %s", p2, p);
				free(p2);
				p2 = p3;
				if (hostfnewer(*src, p)) {
					task->link = true;
					varsetd("in", *src);
					varsetp("out", p);
					sz++;
					arr = xrealloc(arr, sizeof(*arr) * sz);
					arr[sz-1] = varexpand(tc->compile);
					varunset("in");
					varunset("out");
					msgs = xrealloc(msgs, sizeof(*msgs) * sz);
					if (!verbose)
						asprintf(&msgs[sz-1], "=> Compiling %s", *src);
					else
						msgs[sz-1] = arr[sz-1];
				} else {
					free(p);
				}
			}

			if (arr)
				hostexec(arr, msgs, sz, jobs);

			for (size_t i = 0; i < sz; i++) {
				free(arr[i]);
				if (!verbose)
					free(msgs[i]);
			}
			free(arr);
			free(msgs);

			switch (task->type) {
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
			asprintf(&p, "%s/%s%s", bdir, task->name, p4);
			if (task->link || !hostfexists(p)) {
				if (!verbose)
					printf("=> Linking %s\n", p);
				varsetp("in", p2);
				varsetp("out", p);
				if (task->type != LIB)
					varpfxarr("LIBS", tc->libpfx, task->libs);
				p = varexpand(p3);
				if (verbose)
					puts(p);
				if (system(p))
					err("Linking failed");
				varunset("in");
				varunset("out");
				if (task->type != LIB)
					varunset("lib");
			} else {
				puts("(nothing to do)");
				free(p2);
			}
			free(p);

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

static vec(char*) optparse(int argc, char *argv[]) {
	char *p, *p2;
	size_t sz;

	vec(char*) want = NULL;
	int pflag = 0;
	vec(char*) lflag = NULL;
	vec(char*) gflag = NULL;
	bool Gflag = false;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (strchr("lg", argv[i][1]) && !argv[i+1])
				err("Option '-%c' requires an argument", argv[i][1]);
			switch (argv[i][1]) {
				case 'h':
				case '?':
					printf(
						"usage: %s [options] [var=value]... [var+=value]... [task]...\n"
						"\n"
						"options:\n"
						"  -h         Show this help\n"
						"  -p         Process following var=value args after localscript(s)\n"
						"  -l <file>  Load localscript from path, can be repeated to specify multiple\n"
						"  -g <name>  Load named globalscript\n"
						"  -G         Don't load global.dale\n"
					, argv[0]);
					exit(0);
				case 'p':
					if (pflag)
						fprintf(stderr, "Warning: '-p' option used multiple times\n");
					else
						pflag = i+1;
					break;
				case 'l':
					vec_push(lflag, argv[++i]);
					break;
				case 'g':
					vec_push(gflag, argv[++i]);
					break;
				case 'G':
					Gflag = true;
					break;
				default:
					err("Unknown option '%s'", argv[i]);
			}
		} else if ((p = strstr(argv[i], "+="))) {
			if (!pflag) {
				p2 = xstrndup(argv[i], p - argv[i]);
				varappend(p2, p+2);
			}
		} else if (strchr(argv[i], '=')) {
			if (!pflag) {
				sz = strcspn(argv[i], "=");
				p = xstrndup(argv[i], sz);
				p2 = xstrdup(argv[i] + sz + 1);
				varset(p, p2);
			}
		} else {
			vec_push(want, argv[i]);
		}
	}

	if (!Gflag || gflag) {
		for (char **pdir = hostcfgs(); *pdir; pdir++) {
			if (!Gflag) {
				asprintf(&p, "%s/global.dale", *pdir);
				parsef(p, false);
				free(p);
			}
			if (gflag && !*(pdir+1)) {
				for (size_t i = 0; i < vec_size(gflag); i++) {
					asprintf(&p, "%s/%s.dale", *pdir, gflag[i]);
					parsef(p, true);
					free(p);
				}
			}
		}
		free(gflag);
	}

	if (!lflag) {
		parsef("local.dale", false);
	} else {
		for (size_t i = 0; i < vec_size(lflag); i++)
			parsef(lflag[i], true);
		free(lflag);
	}

	if (pflag) {
		for (int i = pflag; i < argc; i++) {
			if ((p = strstr(argv[i], "+="))) {
				p2 = xstrndup(argv[i], p - argv[i]);
				varappend(p2, p+2);
			} else if (strchr(argv[i], '=')) {
				sz = strcspn(argv[i], "=");
				p = xstrndup(argv[i], sz);
				p2 = xstrdup(argv[i] +  sz + 1);
				varset(p, p2);
			}
		}
	}

	return want;
}

static struct tc *loadtc(void) {
	char *tcname, *lang;
	struct tc *tc = NULL;
	char *p, *p2, *p3;

	lang = vargetnull("_lang");
	if (!lang)
		lang = "c";

	tcname = vargetnull("_tcname");

	for (struct tc *cur = tcs; cur < vec_end(tcs); cur++) {
		if (tcname && strcmp(cur->name, tcname))
			continue;
		if (!cur->find || !cur->objext || !cur->libext || !cur->libpfx || !cur->incpfx || !cur->defpfx || !cur->compile || !cur->linkexe || !cur->linklib || !cur->linkdll) {
			fprintf(stderr, "Warning: Skipping underspecified toolchain '%s'\n", cur->name);
			continue;
		}
		if (strcmp(cur->lang, lang))
			continue;

		for (char **find = cur->find; find < vec_end(cur->find); find++) {
			if ((p3 = strchr(*find, '='))) {
				p = xstrndup(*find, p3 - *find);
				puts(p);
				p2 = upperstr(p);
				free(p);
			} else {
				p3 = NULL;
				p2 = upperstr(*find);
			}
			if (vargetnull(p2)) {
				free(p2);
				continue;
			}
			p = hostfind(p3 ? p3+1 : *find);
			if (p) {
				varset(p2, p);
				if (find+1 == vec_end(cur->find))
					tc = cur;
			} else {
				free(p2);
				for (char **found = find-1; found >= cur->find; found--) {
					p = upperstr(*found);
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
		for (struct tc *cur = tcs; cur < vec_end(tcs); cur++)
			if (!strcmp(cur->lang, lang))
				fprintf(stderr, " %s", cur->name);
		fputs(")\n", stderr);
		exit(1);
	}

	return tc;
}

static char *upperstr(const char *s) {
	char *out = xmalloc(strlen(s) + 1);
	for (size_t i = 0; s[i]; i++)
		out[i] = toupper(s[i]);
	out[strlen(s)] = 0;
	return out;
}

static void varpfxarr(const char *var, const char *pfx, vec(char*) vec) {
	char *out, *tmp;
	if (!vec_size(vec))
		return;
	asprintf(&out, "%s%s ", pfx, vec[0]);
	for (size_t i = 1; i < vec_size(vec); i++) {
		asprintf(&tmp, "%s%s%s ", out, pfx, vec[i]);
		free(out);
		out = tmp;
	}
	varappend(var, out);
	free(out);
}

static void taskvarset(const char *var, const char *name) {
	char *p;
	asprintf(&p, "$%s $%s_%s", var, name, var);
	varsetp(var, varexpand(p));
	free(p);
}

static void globarr(vec(char*) *pvec) {
	vec(char*) vec = *pvec;
	*pvec = NULL;
	for (size_t j = 0; j < vec_size(vec); j++) {
		glob(vec[j], pvec);
		free(vec[j]);
	}
	vec_free(vec);
}
