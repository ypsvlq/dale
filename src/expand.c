#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "dale.h"

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
		char *(*fn)(const char *str);
	} builtins[] = {
		{"find", hostfind},
	};

	enum {NONE, NORMAL, COND, BUILTIN} type;
	static const char brackets[] = {0, '(', '[', '{'};
	static const char cbrackets[] = {0, ')', ']', '}'};
	char *bstack = NULL;
	size_t nbstack = 0;

	char *out, *p, *p2, *p3;
	size_t len, sz;
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
							p2 = builtins[i].fn(p);
							if (p2) {
								sz = strlen(p2);
								out = xrealloc(out, len+sz);
								memcpy(out+len, p2, sz);
								len += sz;
								free(p2);
							}
						}
					}
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
