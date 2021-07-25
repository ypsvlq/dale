#define _XOPEN_SOURCE 700
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
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
	varsetd("dllext", ".so");
#if defined __linux__
	varsetd("linux", "1");
#elif defined __OpenBSD__
	varsetd("openbsd", "1");
#elif defined __NetBSD__
	varsetd("netbsd", "1");
#elif defined __FreeBSD__
	varsetd("freebsd", "1");
#elif defined __DragonFly__
	varsetd("dragonflybsd", "1");
#endif
}

char **hostcfgs(void) {
	static char *cfgs[3];
	char *p;
	cfgs[0] = "/etc/dale/global.dale";
	if ((p = getenv("HOME"))) {
		asprintf(&cfgs[1], "%s/.config/dale/global.dale", p);
		cfgs[2] = NULL;
	} else {
		cfgs[1] = NULL;
	}
	return cfgs;
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

void *hostdopen(const char *path) {
	return opendir(path);
}

char *hostdread(void *dir) {
	struct dirent *ent = readdir(dir);
	if (!ent)
		return NULL;
	if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
		return hostdread(dir);
	return xstrdup(ent->d_name);
}

void hostdclose(void *dir) {
	closedir(dir);
}

bool hostisdir(const char *path) {
	struct stat sb;
	if (stat(path, &sb) == -1)
		err("stat '%s': %s", path, strerror(errno));
	return S_ISDIR(sb.st_mode);
}

char *hostexecout(const char *cmd) {
	char *s = NULL;
	size_t sz = 0;
	FILE *p;
	int status;

	p = popen(cmd, "r");
	if (!p)
		err("popen '%s': %s", cmd, strerror(errno));

	errno = 0;
	if (getline(&s, &sz, p) == -1 && errno)
		err("getline: %s", strerror(errno));

	status = pclose(p);
	if (status == -1) {
		err("pclose: %s", strerror(errno));
	} else if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) == 0) {
			s[strlen(s)-1] = '\0';
			return s;
		} else {
			if (s)
				free(s);
			return NULL;
		}
	} else {
		err("Process terminated unexpectedly");
	}
}
