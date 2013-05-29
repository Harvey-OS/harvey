#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

int
lstat(char *name, struct stat *ans)
{
	return stat(name, ans);
}

int
symlink(char *, char *)
{
	errno = EPERM;
	return -1;
}

int
readlink(char *, char *, int )
{
	errno = EIO;
	return -1;
}
