#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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

void varappend(const char *name, const char *val) {
	char *p;
	uint32_t idx = getidx(name);
	for (struct var *var = tbl[idx]; var; var = var->next) {
		if (!strcmp(var->name, name)) {
			asprintf(&p, "%s %s", var->val, val);
			free(var->val);
			var->val = p;
			return;
		}
	}
	varsetd(name, val);
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
	return "";
}

char *vargetnull(const char *name) {
	uint32_t idx = getidx(name);
	for (struct var *var = tbl[idx]; var; var = var->next)
		if (!strcmp(var->name, name))
			return var->val;
	return NULL;
}

char *varexpand(const char *str) {
	enum {NONE, NORMAL, COND} type;
	static const char brackets[] = {0, '(', '['};
	static const char cbrackets[] = {0, ')', ']'};
	char *bstack = NULL;
	size_t nbstack = 0;

	char *out, *p, *p2, *p3;
	size_t len, sz, depth;
	bool negate;

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
			if (type == COND) {
				negate = *str == '!';
				if (negate)
					str++;
			}
			if (type == NONE) {
				sz = strspn(str, valid);
				p = xstrndup(str, sz);
				p2 = varget(p);
				free(p);
			} else {
				for (sz = 0; str[sz]; sz++) {
					if (str[sz] == '$') {
						sz++;
						if (strchr("([", str[sz])) {
							bstack = xrealloc(bstack, ++nbstack);
							for (size_t i = 0; i < LEN(brackets); i++) {
								if (str[sz] == brackets[i]) {
									bstack[nbstack-1] = cbrackets[i];
									break;
								}
							}
						}
					}
					if (nbstack) {
						if (str[sz] == bstack[nbstack-1])
							nbstack--;
					} else if (!strchr(valid, str[sz])) {
						break;
					}
				}
				if (nbstack)
					err("Unmatched brackets");
				p = xstrndup(str, sz);
				p3 = varexpand(p);
				p2 = varget(p3);
				free(p);
				free(p3);
			}
			str += sz;
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
				if ((!negate && p2 && *p2 == '1') || (negate && (!p2 || *p2 != '1'))) {
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
			sz = strlen(p2);
			if (sz) {
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
	if (bstack)
		free(bstack);
	if (!out)
		out = xstrdup("");
	else
		out[len] = '\0';
	return out;
}
