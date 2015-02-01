/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

Object	*getobject(Type, Object *);
Object	*getinclude(Object *);
void	childsort(Object *);
void	childenum(Object *);
Object	*newobject(Type, Object *);
void	freeobject(Object *, char *);
void	freetree(Object *);
void	*mymalloc(void *old, int size);
void	addchild(Object *, Object *, char*);
void	addcatparent(Object *, Object *);
void	inittokenlist(void);
void	initparse(void);
void	exit(int);

extern char *startdir;
