#if __STDC_VERSION__ >= 201112L
#include <stdnoreturn.h>
#else
#define noreturn
#endif
#include <stddef.h>
#include <stdio.h>

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
};

extern const char *tasktypes[4];
extern struct task *tasks;
extern size_t ntasks;

extern const char *fname;
extern size_t line;

noreturn void err(const char *fmt, ...);

void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t max);

int asprintf(char **outp, const char *fmt, ...);

void parse(const char *path);

void varset(const char *name, const char *val);
void varunset(const char *name);
char *varget(const char *name);
char *varexpand(const char *str);

void hostinit(void);
void hostquit(void);
void hostmkdir(const char *path);
char *hostfind(const char *name);
