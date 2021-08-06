#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "dale.h"

static char *find(vec(char*));
static char *map(vec(char*));

static size_t bscan(const char *str, char ob, char cb) {
	char search[] = {cb, '$', '\0'};
	size_t sz = 0;
	size_t depth = 1;
	while (depth) {
		sz += strcspn(str+sz, search);
		if (!str[sz]) {
			err("Unterminated expansion");
		} else if (str[sz] == cb) {
			depth--;
			sz++;
		} else if (str[sz+1] == '$') {
			sz += 2;
		} else if (str[sz+1] == ob) {
			depth++;
			sz += 2;
		} else {
			sz++;
		}
	}
	return sz;
}

char *varexpand(const char *str) {
	static const struct builtin {
		char *name;
		char *(*fn)(vec(char*) args);
		size_t minargs;
	} builtins[] = {
		{"find", find, 1},
		{"map", map, 2},
	};

	enum {NONE, NORMAL, COND, BUILTIN} type;
	static const char brackets[] = {0, '(', '[', '{'};
	static const char cbrackets[] = {0, ')', ']', '}'};
	char *bstack = NULL;
	size_t nbstack = 0;

	char *out, *p, *p2, *p3;
	size_t len, sz;
	bool negate;
	vec(char*) args;

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
						if (strchr("([{", str[sz])) {
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
				free(p);
				if (type != BUILTIN) {
					p2 = varget(p3);
					free(p3);
				}
			}
			str += sz;
			if (type == COND || type == BUILTIN) {
				if (!*str || !*++str)
					err("Unterminated expansion");
				sz = bscan(str, brackets[type], cbrackets[type]);
				p = xstrndup(str, sz-1);
				str += sz;
				if (type == COND) {
					if ((!negate && p2 && *p2 == '1') || (negate && (!p2 || *p2 != '1'))) {
						p2 = varexpand(p);
						sz = strlen(p2);
						out = xrealloc(out, len+sz);
						memcpy(out+len, p2, sz);
						len += sz;
						free(p2);
					}
				} else if (type == BUILTIN) {
					for (size_t i = 0; i < LEN(builtins); i++) {
						if (!strcmp(builtins[i].name, p3)) {
							args = NULL;
							p2 = strtok(p, " \t");
							for (size_t j = 0; j < builtins[i].minargs; j++) {
								if (!p2)
									err("Builtin '%s' takes %zu args but got %zu", builtins[i].name, builtins[i].minargs, j);
								vec_push(args, p2);
								p2 = strtok(NULL, " \t");
							}
							if (p2) {
								if (p+sz-1 > p2+strlen(p2))
									p2[strlen(p2)] = ' ';
								vec_push(args, p2);
							}
							p2 = builtins[i].fn(args);
							vec_free(args);
							if (p2) {
								sz = strlen(p2);
								out = xrealloc(out, len+sz);
								memcpy(out+len, p2, sz);
								len += sz;
								free(p2);
							}
							goto builtinfound;
						}
					}
					err("Unknown builtin '%s'", p3);
builtinfound:
					free(p3);
				}
				free(p);
			} else {
				sz = strlen(p2);
				if (sz) {
					out = xrealloc(out, len+sz+1);
					memcpy(out+len, p2, sz);
					len += sz;
				}
			}
			if (type == NORMAL) {
				if (*str != ')')
					err("Unterminated expansion");
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

static char *find(vec(char*) args) {
	return hostfind(args[0]);
}

static char *map(vec(char*) args) {
	char *out = NULL;
	size_t len = 0;
	char *arr, *cur, *p;
	size_t n;

	if (vec_size(args) < 3)
		err("Nothing to map");

	arr = xstrdup(varget(args[0]));
	cur = strtok(arr, " \t");
	while (cur) {
		varsetc(args[1], cur);
		p = varexpand(args[2]);
		n = strlen(p);
		out = xrealloc(out, len+n+2);
		memcpy(out+len, p, n);
		len += n;
		out[len++] = ' ';
		free(p);
		varunset(args[1]);
		cur = strtok(NULL, " \t");
	}

	free(arr);
	if (out)
		out[len] = '\0';
	return out;
}
