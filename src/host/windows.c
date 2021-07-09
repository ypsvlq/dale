#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <inttypes.h>
#include <shlwapi.h>
#include "../dale.h"

static UINT defaultcp;

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
	defaultcp = GetConsoleOutputCP();
	SetConsoleOutputCP(CP_UTF8);
}

void hostquit(void) {
	SetConsoleOutputCP(defaultcp);
}

void hostsetvars(void) {
	varsetd("target", "windows");
	varsetd("windows", "1");
	varsetd("exeext", ".exe");
}

void hostmkdir(const char *path) {
	PWSTR wpath = mbtows(path);
	if (!CreateDirectoryW(wpath, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
		err("Could not create directory '%s': %s", path, winerr());
}

char *hostfind(const char *name) {
	PWSTR wpath;
	WCHAR exts[_MAX_ENV];
	WCHAR *p, *ctx;
	size_t len;

	wpath = xmalloc(MAX_PATH * sizeof(*wpath));
	if (!MultiByteToWideChar(CP_UTF8, 0, name, -1, wpath, MAX_PATH))
		err("MultiByteToWideChar '%s' failed", name);
	if (!GetEnvironmentVariableW(L"PATHEXT", exts, LEN(exts)))
		err("Could not get PATHEXT: %s", winerr());

	len = wcslen(wpath);
	p = wcstok_s(exts, L";", &ctx);
	do {
		if (len + wcslen(p) + 1 >= MAX_PATH)
			continue;
		wcscat(wpath, p);
		if (PathFindOnPathW(wpath, NULL))
			return wstomb(wpath);
		wpath[len] = 0;
	} while ((p = wcstok_s(NULL, L";", &ctx)));

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
