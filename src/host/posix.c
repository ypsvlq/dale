#define _XOPEN_SOURCE 700
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include "../dale.h"

void hostinit(void) {
}

void hostquit(void) {
}

void hostsetvars(void) {
	varsetc("_target", "posix");
	varsetc("_posix", "1");
	varsetc("_dllext", ".so");
#if defined __linux__
	varsetc("_linux", "1");
#elif defined __OpenBSD__
	varsetc("_openbsd", "1");
#elif defined __NetBSD__
	varsetc("_netbsd", "1");
#elif defined __FreeBSD__
	varsetc("_freebsd", "1");
#elif defined __DragonFly__
	varsetc("_dragonflybsd", "1");
#endif
}

char **hostcfgs(void) {
	static char *cfgs[3];
	char *p;
	cfgs[0] = "/etc/dale";
	if ((p = getenv("HOME"))) {
		asprintf(&cfgs[1], "%s/.config/dale", p);
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

bool hostfnewer(const char *path1, const char *path2) {
	struct stat sb1, sb2;
	if (stat(path1, &sb1) == -1)
		err("stat '%s': %s", path1, strerror(errno));
	if (stat(path2, &sb2) == -1) {
		if (errno == ENOENT)
			return true;
		err("stat '%s': %s", path2, strerror(errno));
	}
	if (sb1.st_mtim.tv_sec != sb2.st_mtim.tv_sec)
		return sb1.st_mtim.tv_sec >= sb2.st_mtim.tv_sec;
	else
		return sb1.st_mtim.tv_nsec >= sb2.st_mtim.tv_nsec;
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

struct tdata {
	pthread_mutex_t mtx;
	vec(vec(char*)) cmds;
	char **msgs;
	size_t len;
};

static void *thread(void *tdata) {
	struct tdata *data = tdata;
	vec(char*) cmds;
	int status;
	while (1) {
		if (pthread_mutex_lock(&data->mtx))
			err("pthread_mutex_lock: %s", strerror(errno));
		if (!data->len) {
			pthread_mutex_unlock(&data->mtx);
			return 0;
		}
		puts(*data->msgs++);
		cmds = *data->cmds++;
		data->len--;
		if (pthread_mutex_unlock(&data->mtx))
			err("pthread_mutex_unlock: %s", strerror(errno));
		for (size_t i = 0; i < vec_size(cmds); i++)
			if ((status = system(cmds[i])))
				err("Command failed (exit code %d)", status);
	}
}

void hostexec(vec(vec(char*)) cmds, char **msgs, size_t jobs) {
	pthread_t *threads;
	struct tdata data = {.cmds = cmds, .msgs = msgs, .len = vec_size(cmds)};

	if (!jobs) {
#if defined __linux__ || defined __OpenBSD__ || defined __NetBSD__ || defined __FreeBSD__ || defined __DragonFly__
		jobs = sysconf(_SC_NPROCESSORS_ONLN);
#else
		jobs = 1;
#endif
	}

	if (jobs > data.len)
		jobs = data.len;

	if (pthread_mutex_init(&data.mtx, NULL))
		err("pthread_mutex_init: %s", strerror(errno));

	threads = xmalloc(jobs * sizeof(*threads));
	for (size_t i = 0; i < jobs; i++)
		pthread_create(&threads[i], NULL, thread, &data);
	for (size_t i = 0; i < jobs; i++)
		pthread_join(threads[i], NULL);

	pthread_mutex_destroy(&data.mtx);
	free(threads);
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
