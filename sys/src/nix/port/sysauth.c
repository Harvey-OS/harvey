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
	return strcmp(eve, up->user) == 0;
}

void
sysfversion(Ar0* ar0, va_list list)
{
	Chan *c;
	char *version;
	int fd;
	u32int msize;
	usize nversion;

	/*
	 * int fversion(int fd, int bufsize, char *version, int nversion);
	 * should be
	 * usize fversion(int fd, u32int msize, char *version, usize nversion);
	 */
	fd = va_arg(list, int);
	msize = va_arg(list, u32int);
	version = va_arg(list, char*);
	nversion = va_arg(list, usize);
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
sys_fsession(Ar0* ar0, va_list list)
{
	int fd;
	char *trbuf;

	/*
	 * int fsession(int fd, char trbuf[TICKREQLEN]);
	 *
	 * Deprecated; backwards compatibility only.
	 */
	fd = va_arg(list, int);
	trbuf = va_arg(list, char*);

	USED(fd);
	trbuf = validaddr(trbuf, 1, 1);
	*trbuf = '\0';

	ar0->i = 0;
}

void
sysfauth(Ar0* ar0, va_list list)
{
	Chan *c, *ac;
	char *aname;
	int fd;

	/*
	 * int fauth(int fd, char *aname);
	 */
	fd = va_arg(list, int);
	aname = va_arg(list, char*);

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
long
userwrite(char* a, long n)
{
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
long
hostownerwrite(char* a, long n)
{
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

long
hostdomainwrite(char* a, long n)
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
