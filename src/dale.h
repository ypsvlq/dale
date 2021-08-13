#if __STDC_VERSION__ >= 201112L
#include <stdnoreturn.h>
#else
#define noreturn
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include "vector.h"

#define LEN(x) (sizeof(x) / sizeof(*x))

struct task {
	struct build *build;
	char *type, *name;
	vec(struct taskvar {
		char *name;
		char *val;
	}) vars;
	bool want;
};

struct rule {
	char *name;
	vec(char*) cmds;
};

struct build {
	char *name;
	vec(char*) steps;
	vec(struct rule) rules;
	const char *fname;
	size_t line;
};

struct ebuiltin {
	char *name;
	char *(*fn)(char **);
	size_t nargs;
	bool exprlast;
};

struct pbuiltin {
	char *name;
	void (*fn)(char **);
	size_t nargs;
};

extern vec(struct task) tasks;
extern vec(struct build) builds;

extern const char *fname;
extern size_t line;

extern const char valid[];

extern const struct ebuiltin ebuiltins[];
extern const struct pbuiltin pbuiltins[];

noreturn void err(const char *fmt, ...);

void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t max);
int asprintf(char **outp, const char *fmt, ...);
char *rstrtok(char **sp, const char *delim);
size_t atosz(const char *s);

void glob(char *pattern, vec(char*) *out);

void parsea(vec(char*) vec, const char *name, size_t startline);
void parsef(const char *path, bool required);

void newvarframe(void);
void varset(char *name, char *val);
void varsetp(const char *name, char *val);
void varsetc(const char *name, const char *val);
void varappend(const char *name, const char *val);
void varunset(const char *name);
char *varget(const char *name);
char *vargetnull(const char *name);
char *varexpand(const char *str);

void hostinit(void);
void hostquit(void);
char **hostcfgs(void);
void hostmkdir(const char *path);
bool hostfnewer(const char *path1, const char *path2);
bool hostfexists(const char *path);
void *hostdopen(const char *path);
char *hostdread(void *dir);
void hostdclose(void *dir);
bool hostisdir(const char *path);
void hostexec(vec(vec(char*)) cmds, char **msgs, size_t jobs);
char *hostexecout(const char *cmd);
