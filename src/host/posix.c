#define _XOPEN_SOURCE 700
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "../dale.h"

static char **pathdirs;
static size_t npathdirs;

void hostinit(void) {
	char *path, *p;

	path = getenv("PATH");
	if (path && *path)
		path = xstrdup(path);
	else
		err("PATH unset");
	npathdirs = 1;
	p = path;
	while ((p = strchr(p, ':'))) {
		npathdirs++;
		p++;
	}
	pathdirs = xmalloc(sizeof(*pathdirs) * npathdirs);
	p = strtok(path, ":");
	for (size_t i = 0; i < npathdirs; i++) {
		pathdirs[i] = p;
		p = strtok(NULL, ":");
	}
}

void hostquit(void) {
}

void hostsetvars(void) {
	varsetd("target", "posix");
	varsetd("posix", "1");
}

void hostmkdir(const char *path) {
	if (mkdir(path, 0755) == -1 && errno != EEXIST)
		err("mkdir: %s", strerror(errno));
}

char *hostfind(const char *name) {
	char *buf;
	for (size_t i = 0; pathdirs[i]; i++) {
		asprintf(&buf, "%s/%s", pathdirs[i], name);
		if (!faccessat(AT_FDCWD, buf, X_OK, AT_EACCESS))
			return buf;
		else if (errno != ENOENT)
			err("faccessat '%s': %s", buf, strerror(errno));
		free(buf);
	}
	return NULL;
}

bool hostfnewer(const char *path1, const char *path2) {
	struct stat sb1, sb2;
	if (stat(path1, &sb1) == -1)
		err("stat '%s': %s", path1, strerror(errno));
	if (stat(path2, &sb2) == -1) {
		if (errno == ENOENT)
			return true;
		err("stat '%s': %s", path2, strerror(errno));
	}
	return sb1.st_mtim.tv_sec >= sb2.st_mtim.tv_sec;
}

bool hostfexists(const char *path) {
	return !access(path, F_OK);
}
