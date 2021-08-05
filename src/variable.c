#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "dale.h"

#define BUCKETS 1000

static struct var {
	struct var *next;
	char *name, *val;
	bool freename, freeval;
} *tbl[BUCKETS];

const char valid[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";

static uint32_t hash(const char *str) {
	uint32_t h = 0x811C9DC5;
	const unsigned char *s = (const unsigned char*)str;
	while (*s) {
		h ^= *s++;
		h *= 0x01000193;
	}
	return h;
}

static uint32_t getidx(const char *name) {
	if (strspn(name, valid) != strlen(name))
		err("Invalid variable name '%s'", name);
	return hash(name) % BUCKETS;
}

static void varset_(char *name, char *val, bool freename, bool freeval) {
	uint32_t idx;
	struct var *var;
	idx = getidx(name);
	var = xmalloc(sizeof(*var));
	var->next = tbl[idx];
	var->name = name;
	var->val = val;
	var->freename = freename;
	var->freeval = freeval;
	tbl[idx] = var;
}

void varset(char *name, char *val) {
	varset_(name, val, true, true);
}

void varsetp(const char *name, char *val) {
	varset_((char*)name, val, false, true);
}

void varsetc(const char *name, const char *val) {
	varset_((char*)name, (char*)val, false, false);
}

void varappend(const char *name, const char *val) {
	char *p;
	uint32_t idx = getidx(name);
	for (struct var *var = tbl[idx]; var; var = var->next) {
		if (!strcmp(var->name, name)) {
			asprintf(&p, "%s %s", var->val, val);
			if (var->freeval)
				free(var->val);
			var->val = p;
			return;
		}
	}
	varset(xstrdup(name), xstrdup(val));
}

void varunset(const char *name) {
	struct var *tmp = NULL;
	uint32_t idx = getidx(name);
	if (tbl[idx]) {
		if (!strcmp(tbl[idx]->name, name)) {
			tmp = tbl[idx];
			tbl[idx] = tbl[idx]->next;
		} else {
			for (struct var *var = tbl[idx]; var->next; var = var->next) {
				if (!strcmp(var->next->name, name)) {
					tmp = var->next;
					var->next = var->next->next;
				}
			}
		}
	}
	if (tmp) {
		if (tmp->freename)
			free(tmp->name);
		if (tmp->freeval)
			free(tmp->val);
		free(tmp);
	}
}

char *varget(const char *name) {
	uint32_t idx = getidx(name);
	for (struct var *var = tbl[idx]; var; var = var->next)
		if (!strcmp(var->name, name))
			return var->val;
	return "";
}

char *vargetnull(const char *name) {
	uint32_t idx = getidx(name);
	for (struct var *var = tbl[idx]; var; var = var->next)
		if (!strcmp(var->name, name))
			return var->val;
	return NULL;
}
