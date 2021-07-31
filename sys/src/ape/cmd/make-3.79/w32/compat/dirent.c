#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "dirent.h"


DIR*
opendir(const char* pDirName)
{
	struct stat sb;
	DIR*	pDir;
	char*	pEndDirName;
	int	nBufferLen;

	/* sanity checks */
	if (!pDirName) {
		errno = EINVAL;
		return NULL;
	}
	if (stat(pDirName, &sb) != 0) {
		errno = ENOENT;
		return NULL;
	}
	if ((sb.st_mode & S_IFMT) != S_IFDIR) {
		errno = ENOTDIR;
		return NULL;
	}

	/* allocate a DIR structure to return */
	pDir = (DIR *) malloc(sizeof (DIR));

	if (!pDir)
		return NULL;

	/* input directory name length */
	nBufferLen = strlen(pDirName);

	/* copy input directory name to DIR buffer */
	strcpy(pDir->dir_pDirectoryName, pDirName);

	/* point to end of the copied directory name */
	pEndDirName = &pDir->dir_pDirectoryName[nBufferLen - 1];

	/* if directory name did not end in '/' or '\', add '/' */
	if ((*pEndDirName != '/') && (*pEndDirName != '\\')) {
		pEndDirName++;
		*pEndDirName = '/';
	}

	/* now append the wildcard character to the buffer */
	pEndDirName++;
	*pEndDirName = '*';
	pEndDirName++;
	*pEndDirName = '\0';

	/* other values defaulted */
	pDir->dir_nNumFiles = 0;
	pDir->dir_hDirHandle = INVALID_HANDLE_VALUE;	
	pDir->dir_ulCookie = __DIRENT_COOKIE;

	return pDir;
}

void
closedir(DIR *pDir)
{
	/* got a valid pointer? */
	if (!pDir) {
		errno = EINVAL;
		return;
	}

	/* sanity check that this is a DIR pointer */
	if (pDir->dir_ulCookie != __DIRENT_COOKIE) {
		errno = EINVAL;
		return;
	}

	/* close the WINDOWS32 directory handle */
	if (pDir->dir_hDirHandle != INVALID_HANDLE_VALUE)
		FindClose(pDir->dir_hDirHandle);

	free(pDir);

	return;
}

struct dirent *
readdir(DIR* pDir)
{
	WIN32_FIND_DATA wfdFindData;

	if (!pDir) {
		errno = EINVAL;
		return NULL;
	}

	/* sanity check that this is a DIR pointer */
	if (pDir->dir_ulCookie != __DIRENT_COOKIE) {
		errno = EINVAL;
		return NULL;
	}

	if (pDir->dir_nNumFiles == 0) {
		pDir->dir_hDirHandle = FindFirstFile(pDir->dir_pDirectoryName, &wfdFindData);
		if (pDir->dir_hDirHandle == INVALID_HANDLE_VALUE)
			return NULL;
	} else if (!FindNextFile(pDir->dir_hDirHandle, &wfdFindData))
			return NULL;

	/* bump count for next call to readdir() or telldir() */
	pDir->dir_nNumFiles++;

	/* fill in struct dirent values */
	pDir->dir_sdReturn.d_ino = -1;
	strcpy(pDir->dir_sdReturn.d_name, wfdFindData.cFileName);

	return &pDir->dir_sdReturn;
}

void
rewinddir(DIR* pDir)
{
	if (!pDir) {
		errno = EINVAL;
		return;
	}

	/* sanity check that this is a DIR pointer */
	if (pDir->dir_ulCookie != __DIRENT_COOKIE) {
		errno = EINVAL;
		return;
	}

	/* close the WINDOWS32 directory handle */
	if (pDir->dir_hDirHandle != INVALID_HANDLE_VALUE)
		if (!FindClose(pDir->dir_hDirHandle))
			errno = EBADF;

	/* reset members which control readdir() */
	pDir->dir_hDirHandle = INVALID_HANDLE_VALUE;
	pDir->dir_nNumFiles = 0;

	return;
}

int
telldir(DIR* pDir)
{
	if (!pDir) {
		errno = EINVAL;
		return -1;
	}

	/* sanity check that this is a DIR pointer */
	if (pDir->dir_ulCookie != __DIRENT_COOKIE) {
		errno = EINVAL;
		return -1;
	}

	/* return number of times readdir() called */
	return pDir->dir_nNumFiles;
}

void
seekdir(DIR* pDir, long nPosition)
{
	if (!pDir)
		return;

	/* sanity check that this is a DIR pointer */
	if (pDir->dir_ulCookie != __DIRENT_COOKIE)
		return;

	/* go back to beginning of directory */
	rewinddir(pDir);

	/* loop until we have found position we care about */
	for (--nPosition; nPosition && readdir(pDir); nPosition--);

	/* flag invalid nPosition value */
	if (nPosition)
		errno = EINVAL;

	return;
}
