#include	<sys/types.h>
#include	<dirent.h>

#undef	rewinddir

void
rewinddir(DIR *dirp)
{
	seekdir(dirp, 0L);
}
