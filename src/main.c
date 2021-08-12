#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "dale.h"

vec(struct task) tasks;
vec(struct build) builds;

static vec(char*) optparse(int argc, char *argv[]);

int main(int argc, char *argv[]) {
	vec(char*) want;
	size_t taskn = 1;
	char *bscript;

	hostinit();

	want = optparse(argc, argv);

	if (*varget("_nodefvar") != '1')
		hostsetvars();

	bscript = vargetnull("_bscript");
	if (!bscript)
		bscript = "build.dale";
	parsef(bscript, true);

	for (size_t i = 0; i < vec_len(want); i++) {
		for (struct task *task = tasks; task < vec_end(tasks); task++) {
			if (!strcmp(task->name, want[i])) {
				task->want = true;
				goto wantfound;
			}
		}
		err("Unknown task '%s'", want[i]);
wantfound:;
	}

	for (struct task *task = tasks; task < vec_end(tasks); task++) {
		if (want && !task->want)
			continue;

		printf("[%zu/%zu] %s\n", taskn++, want ? vec_len(want) : vec_len(tasks), task->name);

		newvarframe();
		varsetc("_task", task->name);
		varsetc("_type", task->type);
		for (struct taskvar *var = task->vars; var < vec_end(task->vars); var++)
			varsetc(var->name, var->val);

		parsea(task->build->steps, task->build->fname, task->build->line);
	}

	hostquit();
}

static vec(char*) optparse(int argc, char *argv[]) {
	char *p;
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
				*p = 0;
				varappend(argv[i], p+2);
			}
		} else if (strchr(argv[i], '=')) {
			if (!pflag) {
				p = strchr(argv[i], '=');
				*p = 0;
				varsetc(argv[i], p+1);
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
			if (!*(pdir+1)) {
				if (gflag) {
					for (size_t i = 0; i < vec_len(gflag); i++) {
						asprintf(&p, "%s/%s.dale", *pdir, gflag[i]);
						parsef(p, true);
						free(p);
					}
				} else {
					asprintf(&p, "%s/default.dale", *pdir);
					parsef(p, false);
					free(p);
				}
			}
		}
		vec_free(gflag);
	}

	if (!lflag) {
		parsef("local.dale", false);
	} else {
		for (size_t i = 0; i < vec_len(lflag); i++)
			parsef(lflag[i], true);
		vec_free(lflag);
	}

	if (pflag) {
		for (int i = pflag; i < argc; i++) {
			if ((p = strstr(argv[i], "+="))) {
				*p = 0;
				varappend(argv[i], p+2);
			} else if (strchr(argv[i], '=')) {
				p = strchr(argv[i], '=');
				*p = 0;
				varsetc(argv[i], p+1);
			}
		}
	}

	return want;
}
