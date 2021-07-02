#include <stdnoreturn.h>
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

void parse(const char *path);
