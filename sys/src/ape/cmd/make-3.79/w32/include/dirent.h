#ifndef _DIRENT_H
#define _DIRENT_H

#include <stdlib.h>
#include <windows.h>
#include <limits.h>
#include <sys/types.h>

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#define __DIRENT_COOKIE 0xfefeabab


struct dirent
{
  ino_t d_ino; 			/* unused - no equivalent on WINDOWS32 */
  char d_name[NAME_MAX+1];
};

typedef struct dir_struct {
	ULONG	dir_ulCookie;
	HANDLE	dir_hDirHandle;
	DWORD	dir_nNumFiles;
	char	dir_pDirectoryName[NAME_MAX+1];
	struct dirent dir_sdReturn;
} DIR;

DIR *opendir(const char *);
struct dirent *readdir(DIR *);
void rewinddir(DIR *);
void closedir(DIR *);
int telldir(DIR *);
void seekdir(DIR *, long);

#endif
