/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stddef.h>
#include <grp.h>

extern int _getpw(int *, char **, char **);
extern char **_grpmems(char *);

static struct group holdgroup;

struct group *
getgrnam(const char *name)
{
	int num;
	char *nam, *mem;

	num = 0;
	nam = (char *)name;
	mem = 0;
	if(_getpw(&num, &nam, &mem)){
		holdgroup.gr_name = nam;
		holdgroup.gr_gid = num;
		holdgroup.gr_mem = _grpmems(mem);
		return &holdgroup;
	}
	return NULL;
}
