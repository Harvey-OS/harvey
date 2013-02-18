#include	"dat.h"
#include	"fns.h"
#include	"error.h"

int		rebootargc = 0;
char**		rebootargv;
extern	char*	hosttype;
extern void libosmain(int argc, char *argv[]);
	int	dflag;
	int vflag;
	int	vflag;
	Procs	procs;
	char	*eve;
	int	bflag = 1;
	int	sflag;
	int	qflag;
char *cputype;

void
srvdynld(void*)
{
	int fd;
	// XXX: move this to OS specific, not portable.
	fd = kcreate("/srv/dynproc", ORDWR, 0775);
	if(fd < 0)
		panic("couldn't srv9: %r");
	if(export(fd, "#L", 0) <= 0)
		panic("couldn't export: %r");
}

void
plan9init(void*)
{
	Osenv *e;
	char *wdir;

	e = up->env;
	e->pgrp = newpgrp();
	e->fgrp = newfgrp(nil);
	e->egrp = newegrp();
	e->errstr = e->errbuf0;
	e->syserrstr = e->errbuf1;
	e->user = strdup("");

	links();
	chandevinit();

	if(waserror())
		panic("setting root and dot");

	e->pgrp->slash = namec("#/", Atodir, 0, 0);
	cnameclose(e->pgrp->slash->name);
	e->pgrp->slash->name = newcname("/");
	e->pgrp->dot = cclone(e->pgrp->slash);
	poperror();

	strcpy(up->text, "main");

	if(kopen("#c/cons", OREAD) != 0)
		fprint(2, "failed to make fd0 from #c/cons: %r\n");
	kopen("#c/cons", OWRITE);
	kopen("#c/cons", OWRITE);

	/* the setid cannot precede the bind of #U */
	kbind("#U", "/", MAFTER|MCREATE);
	setid(eve, 0);
	kbind("#^", "/dev", MBEFORE);	/* snarf */
	kbind("#^", "/chan", MBEFORE);
	kbind("#m", "/dev", MBEFORE);	/* pointer */
	kbind("#c", "/dev", MBEFORE);
	kbind("#p", "/prog", MREPL);
	kbind("#d", "/fd", MREPL);
	kbind("#I", "/net", MAFTER);	/* will fail on Plan 9 */
	
	kbind("#₪", "/srv", MBEFORE|MCREATE);	/* will fail anywhere but plan9 */

	/* BUG: we actually only need to do these on Plan 9 */
	kbind("#U/dev", "/dev", MAFTER);
	kbind("#U/net", "/net", MAFTER);
	kbind("#U/net.alt", "/net.alt", MAFTER);

	if(cputype != nil)
		ksetenv("cputype", cputype, 1);
//	putenvqv("emuargs", rebootargv, rebootargc, 1);
//	putenvq("emuroot", rootdir, 1);
	ksetenv("emuhost", hosttype, 1);
	wdir = malloc(1024);
/*
	if(wdir != nil){
		if(getwd(wdir, 1024) != nil)
			putenvq("emuwdir", wdir, 1);
		free(wdir);
	}
*/

	kproc("main", libosinit, nil, KPDUPFDG|KPDUPPG|KPDUPENVG);
{
	int fd;
	// XXX: move this to OS specific, not portable.
	fd = kcreate("/srv/dynproc", ORDWR, 0775);
	if(fd < 0)
		panic("couldn't srv9: %r");
	if(export(fd, "#L", 0) <= 0)
		panic("couldn't export: %r");
}
	for(;;)
		ospause(); 
}
#undef main
void
main(int argc, char *argv[])
{
	static int started = 0;
	int fd;
	
	// XXX: handle dflag and sflag here, like threadmain
//	if(!started){
	if(1){
		libinit("notdis");
		plan9init("notemu");
		// XXX: Current world never reaches here.
		started = 1;
	}
	/*
	Need a process first
	Need to set up a process group
	Need to set up the errbufs
	Need to set up the user.
	
	XXX: What will we need to import regular plan9 stuff?
	so how do we do system calls?
	there are two ways. 
	we can make a thread.
	Or we can do it with locking.
	Locking will be easier in the beginning I think.
	Let's see how inferno handles the concurrency.
	It might already be handled.
	*/
	libosmain(argc, argv);
}
#define main libosmain

void
errorf(char *fmt, ...)
{
	va_list arg;
	char buf[PRINTSIZE];

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	p9error(buf);
}

void
p9error(char *err)
{
	if(err != up->env->errstr && up->env->errstr != nil)
		kstrcpy(up->env->errstr, err, ERRMAX);
//	ossetjmp(up->estack[NERR-1]);
	nexterror();
}

void
exhausted(char *resource)
{
	char buf[64];
	int n;

	n = snprint(buf, sizeof(buf), "no free %s\n", resource);
	iprint(buf);
	buf[n-1] = 0;
	p9error(buf);
}

void
nexterror(void)
{
	oslongjmp(nil, up->estack[--up->nerr], 1);
}

/* for dynamic modules - functions not macros */

void*
waserr(void)
{
	up->nerr++;
	return up->estack[up->nerr-1];
}

void
poperr(void)
{
	up->nerr--;
}

char*
enverror(void)
{
	return up->env->errstr;
}

void
panic(char *fmt, ...)
{
	va_list arg;
	char buf[512];

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	fprint(2, "panic: %s\n", buf);
	if(sflag)
		abort();

	cleanexit(0);
}

int
iprint(char *fmt, ...)
{

	int n;	
	va_list va;
	char buf[1024];

	va_start(va, fmt);
	n = vseprint(buf, buf+sizeof buf, fmt, va) - buf;
	va_end(va);

	write(1, buf, n);
	return 1;
}

void
_assert(char *fmt)
{
	panic("assert failed: %s", fmt);
}

/*
 * mainly for libmp
 */
void
sysfatal(char *fmt, ...)
{
	va_list arg;
	char buf[64];

	va_start(arg, fmt);
	vsnprint(buf, sizeof(buf), fmt, arg);
	va_end(arg);
	p9error(buf);
}

void
oserror(void)
{
	oserrstr(up->env->errstr, ERRMAX);
	p9error(up->env->errstr);
}
