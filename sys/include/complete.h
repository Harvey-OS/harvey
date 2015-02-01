/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma	lib	"libcomplete.a"
#pragma src "/sys/src/libcomplete"

typedef struct Completion Completion;

struct Completion{
	uchar advance;		/* whether forward progress has been made */
	uchar complete;	/* whether the completion now represents a file or directory */
	char *string;		/* the string to advance, suffixed " " or "/" for file or directory */
	int nmatch;		/* number of files that matched */
	int nfile;			/* number of files returned */
	char **filename;	/* their names */
};

Completion* complete(char *dir, char *s);
void freecompletion(Completion*);
