#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include "../dale.h"

void hostmkdir(const char *path) {
	if (mkdir(path, 0755) == -1 && errno != EEXIST)
		err("mkdir: %s", strerror(errno));
}
