/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct File File;
typedef struct Fs Fs;

#include "dosfs.h"
#include "kfs.h"

struct File {
	union {
		Dosfile dos;
		Kfsfile kfs;
		int walked;
	};
	Fs *fs;
	char *path;
};

struct Fs {
	union {
		Dos dos;
		Kfs kfs;
	};
	int dev;			      /* device id */
	long (*diskread)(Fs *, void *, long); /* disk read routine */
	vlong (*diskseek)(Fs *, vlong);       /* disk seek routine */
	long (*read)(File *, void *, long);
	int (*walk)(File *, char *);
	File root;
};

extern int chatty;
extern int dotini(Fs *);
extern int fswalk(Fs *, char *, File *);
extern int fsread(File *, void *, long);
extern int fsboot(Fs *, char *, Boot *);

#define BADPTR(x) ((uint32_t)x < (uint32_t)(KZERO + 0x7c00))
