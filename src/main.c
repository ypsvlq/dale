#include <stdio.h>
#include "dale.h"

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	FILE *f;

	f = fopen("build.dale", "r");
	if (!f)
		err("Could not open build.dale");
	fclose(f);
}
