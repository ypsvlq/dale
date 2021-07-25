#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <inttypes.h>
#include <shlwapi.h>
#include "../dale.h"

static UINT defaultcp;
static WCHAR **pathexts;
static size_t npathexts;

static PWSTR mbtows(const char *s) {
	PWSTR ws;
	int n;
	n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
	if (!n)
		err("MultiByteToWideChar '%s' failed", s);
	ws = xmalloc(n * sizeof(*ws));
	if (!MultiByteToWideChar(CP_UTF8, 0, s, -1, ws, n))
		err("MultiByteToWideChar '%s' failed", s);
	return ws;
}

static char *wstomb(PCWSTR ws) {
	char *s;
	int n;
	n = WideCharToMultiByte(CP_UTF8, 0, ws, -1, NULL, 0, NULL, NULL);
	if (!n)
		err("WideCharToMultiByte '%ls' failed", ws);
	s = xmalloc(n);
	if (!WideCharToMultiByte(CP_UTF8, 0, ws, -1, s, n, NULL, NULL))
		err("WideCharToMultiByte '%ls' failed", ws);
	return s;
}

static char *winerr(void) {
	LPVOID buf;
	if (!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buf, 0, NULL)) {
		switch (GetLastError()) {
			case ERROR_MORE_DATA:
				break;
			default:
				asprintf((char**)&buf, "FormatMessage failed (code %08"PRIx32")", GetLastError());
				return buf;
		}
	}
	return wstomb(buf);
}

void hostinit(void) {
	static WCHAR exts[_MAX_ENV];
	WCHAR *p, *ctx;

	defaultcp = GetConsoleOutputCP();
	SetConsoleOutputCP(CP_UTF8);

	if (!GetEnvironmentVariableW(L"PATHEXT", exts, LEN(exts)))
		err("Could not get PATHEXT: %s", winerr());
	p = exts;
	do {
		npathexts++;
		p++;
	} while ((p = wcschr(p, L';')));
	pathexts = xmalloc(sizeof(*pathexts) * npathexts);
	p = wcstok_s(exts, L";", &ctx);
	for (size_t i = 0; i < npathexts; i++) {
		pathexts[i] = p;
		p = wcstok_s(NULL, L";", &ctx);
	}
}

void hostquit(void) {
	SetConsoleOutputCP(defaultcp);
}

void hostsetvars(void) {
	varsetd("target", "windows");
	varsetd("windows", "1");
	varsetd("exeext", ".exe");
	varsetd("dllext", ".dll");
}

char **hostcfgs(void) {
	static char *cfgs[2];
	char *p;
	if ((p = getenv("APPDATA"))) {
		asprintf(&cfgs[0], "%s/Dale/global.dale", p);
		cfgs[1] = NULL;
	} else {
		cfgs[0] = NULL;
	}
	return cfgs;
}

void hostmkdir(const char *path) {
	PWSTR wpath = mbtows(path);
	if (!CreateDirectoryW(wpath, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
		err("Could not create directory '%s': %s", path, winerr());
	free(wpath);
}

char *hostfind(const char *name) {
	PWSTR wpath;
	size_t len;

	wpath = xmalloc(MAX_PATH * sizeof(*wpath));
	if (!MultiByteToWideChar(CP_UTF8, 0, name, -1, wpath, MAX_PATH))
		err("MultiByteToWideChar '%s' failed", name);

	len = wcslen(wpath);
	for (size_t i = 0; i < npathexts; i++) {
		if (len + wcslen(pathexts[i]) + 1 >= MAX_PATH)
			continue;
		wcscat(wpath, pathexts[i]);
		if (PathFindOnPathW(wpath, NULL))
			return wstomb(wpath);
		wpath[len] = 0;
	}

	free(wpath);
	return NULL;
}

bool hostfnewer(const char *path1, const char *path2) {
	PWSTR wpath1, wpath2;
	HANDLE f1, f2;
	FILETIME t1, t2;
	wpath1 = mbtows(path1);
	wpath2 = mbtows(path2);
	f1 = CreateFileW(wpath1, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f1 == INVALID_HANDLE_VALUE)
		err("Could not open '%s': %s", path1, winerr());
	f2 = CreateFileW(wpath2, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f2 == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			CloseHandle(f1);
			free(wpath1);
			free(wpath2);
			return true;
		} else {
			err("Could not open '%s': %s", path2, winerr());
		}
	}
	GetFileTime(f1, NULL, NULL, &t1);
	GetFileTime(f2, NULL, NULL, &t2);
	CloseHandle(f1);
	CloseHandle(f2);
	free(wpath1);
	free(wpath2);
	return (CompareFileTime(&t1, &t2) > -1);
}

bool hostfexists(const char *path) {
	PWSTR wpath;
	HANDLE f;
	wpath = mbtows(path);
	f = CreateFileW(wpath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_NOT_FOUND)
		err("Could not open '%s': %s", path, winerr());
	CloseHandle(f);
	free(wpath);
	return f != INVALID_HANDLE_VALUE;
}

struct dir {
	HANDLE h;
	WIN32_FIND_DATAW data;
	char *pattern;
	bool done;
};

void *hostdopen(const char *path) {
	struct dir *d;
	PWSTR wpattern;

	d = xmalloc(sizeof(*d));
	asprintf(&d->pattern, "%s/*", path);
	wpattern = mbtows(d->pattern);

	d->done = false;
	d->h = FindFirstFileW(wpattern, &d->data);
	if (d->h == INVALID_HANDLE_VALUE)
		err("Could not read directory '%s': %s", d->pattern, winerr());

	free(wpattern);
	return d;
}

char *hostdread(void *dir) {
	struct dir *d = dir;
	char *path;

	if (d->done)
		return NULL;

	path = wstomb(d->data.cFileName);
	if (!FindNextFileW(d->h, &d->data)) {
		if (GetLastError() == ERROR_NO_MORE_FILES)
			d->done = true;
		else
			err("Could not read directory '%s': %s", d->pattern, winerr());
	}

	return path;
}

void hostdclose(void *dir) {
	struct dir *d = dir;
	FindClose(d->h);
	free(d->pattern);
	free(d);
}

bool hostisdir(const char *path) {
	PWSTR wpath = mbtows(path);
	bool ret = GetFileAttributesW(wpath) & FILE_ATTRIBUTE_DIRECTORY;
	free(wpath);
	return ret;
}

char *hostexecout(const char *cmd) {
	char *s, *s2;
	FILE *p;

	p = _popen(cmd, "r");
	if (!p)
		err("popen '%s': %s", cmd, strerror(errno));

	s = xmalloc(8192);
	s2 = fgets(s, 8192, p);
	if (s2)
		s[strlen(s)-1] = '\0';

	if (_pclose(p)) {
		free(s);
		return NULL;
	} else {
		return s;
	}
}
