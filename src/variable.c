#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "dale.h"

#define BUCKETS 1000

static struct var {
	struct var *next;
	char *name, *val;
} *tbl[BUCKETS];

static const char valid[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";

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

void varset(char *name, char *val) {
	uint32_t idx;
	struct var *var;
	idx = getidx(name);
	var = xmalloc(sizeof(*var));
	var->next = tbl[idx];
	var->name = name;
	var->val = val;
	tbl[idx] = var;
}

void varsetp(const char *name, char *val) {
	varset(xstrdup(name), val);
}

void varsetd(const char *name, const char *val) {
	varsetp(name, xstrdup(val));
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
		free(tmp->name);
		free(tmp->val);
		free(tmp);
	}
}

char *varget(const char *name) {
	uint32_t idx = getidx(name);
	for (struct var *var = tbl[idx]; var; var = var->next)
		if (!strcmp(var->name, name))
			return var->val;
	return NULL;
}

char *varexpand(const char *str) {
	enum {NONE, NORMAL, COND} type;
	static const char brackets[] = {0, '(', '['};

	char *out, *p, *p2;
	size_t len, sz, depth;

	out = NULL;
	len = 0;
	while (1) {
		sz = strcspn(str, "$");
		out = xrealloc(out, len+sz+1);
		memcpy(out+len, str, sz);
		len += sz;
		str += sz;
		if (!*str) {
			break;
		} else if (*(str+1) == '$') {
			str += 2;
			out = xrealloc(out, len+2);
			out[len++] = '$';
		} else {
			str++;
			type = NONE;
			for (size_t i = 1; i < LEN(brackets); i++) {
				if (*str == brackets[i]) {
					type = i;
					str++;
					break;
				}
			}
			sz = strspn(str, valid);
			p = xstrndup(str, sz);
			p2 = varget(p);
			str += sz;
			free(p);
			if (type == COND) {
				if (!*str || !*++str)
					err("Unterminated conditional expansion");
				sz = 0;
				depth = 1;
				while (depth) {
					sz += strcspn(str+sz, "$]");
					if (!str[sz]) {
						err("Unterminated conditional expansion");
					} else if (str[sz] == ']') {
						depth--;
						sz++;
					} else if (str[sz+1] == '$') {
						sz += 2;
					} else if (str[sz+1] == '[') {
						depth++;
						sz += 2;
					} else {
						sz++;
					}
				}
				p = xstrndup(str, sz-1);
				str += sz;
				if (p2 && *p2 == '1') {
					p2 = varexpand(p);
					sz = strlen(p2);
					out = xrealloc(out, len+sz);
					memcpy(out+len, p2, sz);
					len += sz;
					free(p2);
				}
				free(p);
				continue;
			}
			if (p2) {
				sz = strlen(p2);
				out = xrealloc(out, len+sz+1);
				memcpy(out+len, p2, sz);
				len += sz;
			}
			if (type == NORMAL) {
				if (*str != ')')
					err("Unterminated variable expansion");
				str++;
			}
		}
	}
	if (!len)
		out = xstrdup("");
	else
		out[len] = '\0';
	return out;
}
