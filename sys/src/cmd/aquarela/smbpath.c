#include "headers.h"

void
smbpathsplit(char *path, char **dirp, char **namep)
{
	char *dir;
	char *p = strrchr(path, '/');
	if (p == nil) {
		*dirp = smbestrdup("/");
		*namep = smbestrdup(path);
		return;
	}
	if (p == path)
		dir = smbestrdup("/");
	else {
		dir = smbemalloc(p - path + 1);
		memcpy(dir, path, p - path);
		dir[p - path] = 0;
	}
	*dirp = dir;
	*namep = smbestrdup(p + 1);
}
