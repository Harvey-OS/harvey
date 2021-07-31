#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

int
lstat(char *name, struct stat *ans)
{
	return stat(name, ans);
}

int
symlink(char *name1, char *name2)
{
	errno = EPERM;
	return -1;
}

int
readlink(char *name, char *buf, int size)
{
	errno = EIO;
	return -1;
}
