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
#include <draw.h>

/*
 * Default version: treat as file name
 */

Subfont*
_getsubfont(Display *d, char *name)
{
	int fd;
	Subfont *f;

	fd = open(name, OREAD);
		
	if(fd < 0){
		fprint(2, "getsubfont: can't open %s: %r\n", name);
		return 0;
	}
	/*
	 * unlock display so i/o happens with display released, unless
	 * user is doing his own locking, in which case this could break things.
	 * _getsubfont is called only from string.c and stringwidth.c,
	 * which are known to be safe to have this done.
	 */
	if(d && d->locking == 0)
		unlockdisplay(d);
	f = readsubfont(d, name, fd, d && d->locking==0);
	if(d && d->locking == 0)
		lockdisplay(d);
	if(f == 0)
		fprint(2, "getsubfont: can't read %s: %r\n", name);
	close(fd);
	setmalloctag(f, getcallerpc(&d));
	return f;
}
