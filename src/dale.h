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
	enum tasktypes {
		EXE=1,
		LIB,
		DLL,
	} type;
	char *name;
	vec(char*) srcs;
	vec(char*) libs;
	vec(char*) incs;
	vec(char*) defs;
	vec(char*) reqs;
	bool build, link;
};

struct tc {
	char *lang;
	char *name;
	vec(char*) find;
	char *objext, *libext;
	char *libpfx, *incpfx, *defpfx;
	char *compile, *linkexe, *linklib, *linkdll;
};

extern vec(struct task) tasks;
extern vec(struct tc) tcs;

extern const char *fname;
extern size_t line;

extern const char valid[];

noreturn void err(const char *fmt, ...);

void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t max);
int asprintf(char **outp, const char *fmt, ...);

void glob(char *pattern, vec(char*) *out);

void parsea(const char *arr[], size_t len);
void parsef(const char *path, bool required);

void varset(char *name, char *val);
void varsetp(const char *name, char *val);
void varsetd(const char *name, const char *val);
void varappend(const char *name, const char *val);
void varunset(const char *name);
char *varget(const char *name);
char *vargetnull(const char *name);
char *varexpand(const char *str);

void hostinit(void);
void hostquit(void);
void hostsetvars(void);
char **hostcfgs(void);
void hostmkdir(const char *path);
char *hostfind(const char *name);
bool hostfnewer(const char *path1, const char *path2);
bool hostfexists(const char *path);
void *hostdopen(const char *path);
char *hostdread(void *dir);
void hostdclose(void *dir);
bool hostisdir(const char *path);
void hostexec(char **cmds, char **msgs, size_t len, size_t jobs);
char *hostexecout(const char *cmd);
