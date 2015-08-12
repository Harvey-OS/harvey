/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "fs.h"

/*
 *  grab next element from a path, return the pointer to unprocessed portion of
 *  path.
 */
char *
nextelem(char *path, char *elem)
{
	int i;

	while(*path == '/')
		path++;
	if(*path == 0 || *path == ' ')
		return 0;
	for(i = 0; *path != '\0' && *path != '/' && *path != ' '; i++) {
		if(i == NAMELEN) {
			print("name component too long\n");
			return 0;
		}
		*elem++ = *path++;
	}
	*elem = '\0';
	return path;
}

int
fswalk(Fs *fs, char *path, File *f)
{
	char element[NAMELEN];

	*f = fs->root;
	if(BADPTR(fs->walk))
		panic("fswalk bad pointer fs->walk");

	f->path = path;
	while(path = nextelem(path, element)) {
		switch(fs->walk(f, element)) {
		case -1:
			return -1;
		case 0:
			return 0;
		}
	}
	return 1;
}

/*
 *  boot
 */
int
fsboot(Fs *fs, char *path, Boot *b)
{
	File file;
	int32_t n;
	static char buf[8192];

	switch(fswalk(fs, path, &file)) {
	case -1:
		print("error walking to %s\n", path);
		return -1;
	case 0:
		print("%s not found\n", path);
		return -1;
	case 1:
		print("found %s\n", path);
		break;
	}

	while((n = fsread(&file, buf, sizeof buf)) > 0) {
		if(bootpass(b, buf, n) != MORE)
			break;
	}

	bootpass(b, nil, 0); /* tries boot */
	return -1;
}

int
fsread(File *file, void *a, int32_t n)
{
	if(BADPTR(file->fs))
		panic("bad pointer file->fs in fsread");
	if(BADPTR(file->fs->read))
		panic("bad pointer file->fs->read in fsread");
	return file->fs->read(file, a, n);
}
