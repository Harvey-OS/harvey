/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>

static Lock	privlock;
static int	privinit;
static void	**privs;

extern void	**_privates;
extern int	_nprivates;

void **
privalloc(void)
{
	void **p;
	int i;

	lock(&privlock);
	if(!privinit){
		privinit = 1;
		if(_nprivates){
			_privates[0] = 0;
			for(i = 1; i < _nprivates; i++)
				_privates[i] = &_privates[i - 1];
			privs = &_privates[i - 1];
		}
	}
	p = privs;
	if(p != nil){
		privs = *p;
		*p = nil;
	}
	unlock(&privlock);
	return p;
}

void
privfree(void **p)
{
	lock(&privlock);
	if(p != nil && privinit){
		*p = privs;
		privs = p;
	}
	unlock(&privlock);
}
