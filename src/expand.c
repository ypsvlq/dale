#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "dale.h"

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
