/*
 * as user cmd [arg...] - run cmd with args as user on this cpu server.
 *	must be hostowner for this to work.
 *	needs #¤/caphash and #¤/capuse.
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <libsec.h>
#include <auth.h>
#include "authcmdlib.h"

int	debug;

int	becomeuser(char*);
void	createuser(void);
void	*emalloc(ulong);
void	*erealloc(void*, ulong);
void	initcap(void);
int	mkcmd(char*, char*, int);
int	myauth(int, char*);
int	qidcmp(Qid, Qid);
void	runas(char *, char *);
void	usage(void);

#pragma varargck	argpos clog 1
#pragma varargck	argpos fatal 1

static void
fatal(char *fmt, ...)
{
	char msg[256];
	va_list arg;

	va_start(arg, fmt);
	vseprint(msg, msg + sizeof msg, fmt, arg);
	va_end(arg);
	error("%s", msg);
}

void
main(int argc, char *argv[])
{
	debug = 0;
	ARGBEGIN{
	case 'd':
		debug = 1;
		break;
	default:
		usage();
	}ARGEND

	initcap();
	srand(getpid()*time(0));
	runas(argv[0], argv[1]);
}

void
runas(char *user, char *cmd)
{
	if(becomeuser(user) < 0)
		sysfatal("can't change uid for %s: %r", user);
	putenv("service", "rx");
	execl("/bin/rc", "rc", "-lc", cmd, nil);
	sysfatal("exec /bin/rc: %r");
}

void *
emalloc(ulong n)
{
	void *p;

	if(p = mallocz(n, 1))
		return p;
	fatal("out of memory");
	return 0;
}

void *
erealloc(void *p, ulong n)
{
	if(p = realloc(p, n))
		return p;
	fatal("out of memory");
	return 0;
}

void
usage(void)
{
	fprint(2, "usage: %s [-c]\n", argv0);
	exits("usage");
}

void
memrandom(void *p, int n)
{
	uchar *cp;

	for(cp = (uchar*)p; n > 0; n--)
		*cp++ = fastrand();
}

/*
 *  keep caphash fd open since opens of it could be disabled
 */
static int caphashfd;

void
initcap(void)
{
	caphashfd = open("#¤/caphash", OCEXEC|OWRITE);
	if(caphashfd < 0)
		fprint(2, "%s: opening #¤/caphash: %r\n", argv0);
}

/*
 *  create a change uid capability 
 */
char*
mkcap(char *from, char *to)
{
	uchar rand[20];
	char *cap;
	char *key;
	int nfrom, nto, ncap;
	uchar hash[SHA1dlen];

	if(caphashfd < 0)
		return nil;

	/* create the capability */
	nto = strlen(to);
	nfrom = strlen(from);
	ncap = nfrom + 1 + nto + 1 + sizeof(rand)*3 + 1;
	cap = emalloc(ncap);
	snprint(cap, ncap, "%s@%s", from, to);
	memrandom(rand, sizeof(rand));
	key = cap+nfrom+1+nto+1;
	enc64(key, sizeof(rand)*3, rand, sizeof(rand));

	/* hash the capability */
	hmac_sha1((uchar*)cap, strlen(cap), (uchar*)key, strlen(key), hash, nil);

	/* give the kernel the hash */
	key[-1] = '@';
	if(write(caphashfd, hash, SHA1dlen) < 0){
		free(cap);
		return nil;
	}

	return cap;
}

int
usecap(char *cap)
{
	int fd, rv;

	fd = open("#¤/capuse", OWRITE);
	if(fd < 0)
		return -1;
	rv = write(fd, cap, strlen(cap));
	close(fd);
	return rv;
}

int
becomeuser(char *new)
{
	char *cap;
	int rv;

	cap = mkcap(getuser(), new);
	if(cap == nil)
		return -1;
	rv = usecap(cap);
	free(cap);

	newns(new, nil);
	return rv;
}
