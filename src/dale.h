#if __STDC_VERSION__ >= 201112L
#include <stdnoreturn.h>
#else
#define noreturn
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#define LEN(x) (sizeof(x) / sizeof(*x))

struct task {
	enum tasktypes {
		EXE=1,
		LIB,
		DLL,
	} type;
	char *name;
	char **srcs;
	size_t nsrcs;
	char **libs;
	size_t nlibs;
	bool build, link;
};

struct tc {
	char *name;
	char **find;
	size_t nfind;
	char *objext, *libprefix;
	char *compile, *linkexe, *linklib, *linkdll;
};

extern const char *tasktypes[4];
extern struct task *tasks;
extern size_t ntasks;
extern struct tc *tcs;
extern size_t ntcs;

extern const char *fname;
extern size_t line;

noreturn void err(const char *fmt, ...);

void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t max);

int asprintf(char **outp, const char *fmt, ...);
char *upperstr(const char *s);

void glob(char *pattern, char ***out, size_t *outsz);

void parsea(const char *arr[], size_t len);
void parsef(const char *path, bool required);

void varset(char *name, char *val);
void varsetp(const char *name, char *val);
void varsetd(const char *name, const char *val);
void varunset(const char *name);
void varupdate(const char *name, char *val);
char *varget(const char *name);
char *vargetnull(const char *name);
char *varexpand(const char *str);

void hostinit(void);
void hostquit(void);
void hostsetvars(void);
void hostmkdir(const char *path);
char *hostfind(const char *name);
bool hostfnewer(const char *path1, const char *path2);
bool hostfexists(const char *path);
void *hostdopen(const char *path);
char *hostdread(void *dir);
void hostdclose(void *dir);
bool hostisdir(const char *path);
