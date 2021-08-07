#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dale.h"

static void dump(char **);

const struct pbuiltin pbuiltins[] = {
	{"dump", dump, 1},
	{0}
};

static void dump(char **args) {
	printf("%s = %s\n", args[0], varget(args[0]));
}
