/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Glob Glob;
typedef struct Globlist Globlist;

struct Glob{
	String	*glob;
	Glob	*next;
};

struct Globlist{
	Glob	*first;
	Glob	**l;
};

extern	Globlist*	glob(char*);
extern	void		globadd(Globlist*, char*, char*);
extern	void		globlistfree(Globlist *gl);
extern	char*		globiter(Globlist *gl);
