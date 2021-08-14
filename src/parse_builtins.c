#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dale.h"

static void dump(char **);
static void do_(char **);
static void mkdirs(char **);

const struct pbuiltin pbuiltins[] = {
	{"dump", dump, 1},
	{"do", do_, 3},
	{"mkdirs", mkdirs, 1},
	{0}
};

static void dump(char **args) {
	printf("%s = %s\n", args[0], varget(args[0]));
}

static void do_(char **args) {
	struct build *build = curtask->build;
	char *in, *out;
	char *ictx, *octx, *icur, *ocur;
	bool iarr, oarr, exec;
	char *msg, *tmp;
	vec(vec(char*)) cmds = NULL;
	vec(char*) msgs = NULL;

	for (struct rule *rule = build->rules; rule < vec_end(build->rules); rule++) {
		if (!strcmp(rule->name, args[0])) {
			out = varget(args[1]);
			in = varget(args[2]);

			iarr = strpbrk(in, " \t");
			oarr = strpbrk(out, " \t");

			if (iarr & oarr) {
				in = ictx = xstrdup(in);
				out = octx = xstrdup(out);
				while ((icur = rstrtok(&ictx, " \t"))) {
					ocur = rstrtok(&octx, " \t");
					if (!ocur)
						err("More inputs than outputs");
					if (hostfnewer(icur, ocur)) {
						varsetc("in", icur);
						varsetc("out", ocur);
						vec_push(cmds, NULL);
						for (size_t i = 0; i < vec_len(rule->cmds); i++)
							vec_push(cmds[vec_len(cmds)-1], varexpand(rule->cmds[i]));
						asprintf(&msg, "%s %s", rule->name, icur);
						vec_push(msgs, msg);
						varunset("in");
						varunset("out");
					}
				}
				if ((ocur = rstrtok(&octx, " \t")))
					err("More outputs than inputs");
				free(in);
				free(out);
			} else if (!oarr) {
				if (iarr) {
					exec = false;
					tmp = ictx = xstrdup(in);
					while ((icur = rstrtok(&ictx, " \t"))) {
						if (hostfnewer(icur, out)) {
							exec = true;
							break;
						}
					}
					free(tmp);
				} else {
					exec = hostfnewer(in, out);
				}

				if (exec) {
					varsetc("in", in);
					varsetc("out", out);
					vec_push(cmds, NULL);
					for (size_t i = 0; i < vec_len(rule->cmds); i++)
						vec_push(cmds[0], varexpand(rule->cmds[i]));
					asprintf(&msg, "%s %s", rule->name, iarr ? out : in);
					vec_push(msgs, msg);
					varunset("in");
					varunset("out");
				}
			} else {
				err("More outputs than inputs");
			}

			hostexec(cmds, msgs, jobs);

			for (size_t i = 0; i < vec_len(cmds); i++) {
				for (size_t j = 0; j < vec_len(cmds[i]); j++)
					free(cmds[i][j]);
				vec_free(cmds[i]);
				free(msgs[i]);
			}
			vec_free(cmds);
			vec_free(msgs);
			return;
		}
	}
	err("Undefined rule '%s'", args[0]);
}

static void mkdirs(char **args) {
	char *arr, *path, *cur, *ctxa, *ctxp;
	ctxa = arr = xstrdup(varget(args[0]));
	while ((path = rstrtok(&ctxa, " \t"))) {
		ctxp = path;
		while ((cur = rstrtok(&ctxp, "/"))) {
			if (cur != path)
				cur[-1] = '/';
			hostmkdir(path);
			if (!strchr(ctxp, '/')) {
				ctxp[-1] = '/';
				break;
			}
		}
	}
	free(arr);
}
