#include <stdnoreturn.h>
#include <stddef.h>

noreturn void err(const char *fmt, ...);

void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);
