/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	<authsrv.h>

char	*eve;
char	hostdomain[DOMLEN];

/*
 *  return true if current user is eve
 */
int
iseve(void)
{
	Proc *up = externup();
	return strcmp(eve, up->user) == 0;
}

void
sysfversion(Ar0* ar0, ...)
{
	Proc *up = externup();
	Chan *c;
	char *version;
	int fd;
	uint32_t msize;
	usize nversion;
	va_list list;
	va_start(list, ar0);

	/*
	 * int fversion(int fd, int bufsize, char *version, int nversion);
	 * should be
	 * usize fversion(int fd, u32int msize, char *version, usize nversion);
	 */
	fd = va_arg(list, int);
	msize = va_arg(list, uint32_t);
	version = va_arg(list, char*);
	nversion = va_arg(list, usize);
	va_end(list);
	version = validaddr(version, nversion, 1);
	/* check there's a NUL in the version string */
	if(nversion == 0 || memchr(version, 0, nversion) == nil)
		error(Ebadarg);

	c = fdtochan(fd, ORDWR, 0, 1);
	if(waserror()){
		cclose(c);
		nexterror();
	}

	ar0->u = mntversion(c, msize, version, nversion);

	cclose(c);
	poperror();
}

void
sys_fsession(Ar0* ar0, ...)
{
	int fd;
	char *trbuf;
	va_list list;
	va_start(list, ar0);

	/*
	 * int fsession(int fd, char trbuf[TICKREQLEN]);
	 *
	 * Deprecated; backwards compatibility only.
	 */
	fd = va_arg(list, int);
	trbuf = va_arg(list, char*);
	va_end(list);

	USED(fd);
	trbuf = validaddr(trbuf, 1, 1);
	*trbuf = '\0';

	ar0->i = 0;
}

void
sysfauth(Ar0* ar0, ...)
{
	Proc *up = externup();
	Chan *c, *ac;
	char *aname;
	int fd;
	va_list list;
	va_start(list, ar0);

	/*
	 * int fauth(int fd, char *aname);
	 */
	fd = va_arg(list, int);
	aname = va_arg(list, char*);
	va_end(list);

	aname = validaddr(aname, 1, 0);
	aname = validnamedup(aname, 1);
	if(waserror()){
		free(aname);
		nexterror();
	}
	c = fdtochan(fd, ORDWR, 0, 1);
	if(waserror()){
		cclose(c);
		nexterror();
	}

	ac = mntauth(c, aname);
	/* at this point ac is responsible for keeping c alive */
	cclose(c);
	poperror();	/* c */
	free(aname);
	poperror();	/* aname */

	if(waserror()){
		cclose(ac);
		nexterror();
	}

	fd = newfd(ac);
	if(fd < 0)
		error(Enofd);
	poperror();	/* ac */

	/* always mark it close on exec */
	ac->flag |= CCEXEC;

	ar0->i = fd;
}

/*
 *  called by devcons() for user device
 *
 *  anyone can become none
 */
int32_t
userwrite(char* a, int32_t n)
{
	Proc *up = externup();
	if(n != 4 || strncmp(a, "none", 4) != 0)
		error(Eperm);
	kstrdup(&up->user, "none");
	up->basepri = PriNormal;

	return n;
}

/*
 *  called by devcons() for host owner/domain
 *
 *  writing hostowner also sets user
 */
int32_t
hostownerwrite(char* a, int32_t n)
{
	Proc *up = externup();
	char buf[128];

	if(!iseve())
		error(Eperm);
	if(n <= 0 || n >= sizeof buf)
		error(Ebadarg);
	memmove(buf, a, n);
	buf[n] = 0;

	renameuser(eve, buf);
	kstrdup(&eve, buf);
	kstrdup(&up->user, buf);
	up->basepri = PriNormal;

	return n;
}

int32_t
hostdomainwrite(char* a, int32_t n)
{
	char buf[DOMLEN];

	if(!iseve())
		error(Eperm);
	if(n >= DOMLEN)
		error(Ebadarg);
	memset(buf, 0, DOMLEN);
	strncpy(buf, a, n);
	if(buf[0] == 0)
		error(Ebadarg);
	memmove(hostdomain, buf, DOMLEN);

	return n;
}
