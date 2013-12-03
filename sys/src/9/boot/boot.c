#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "../boot/boot.h"

#define PARTSRV "partfs.sdXX"

enum {
	Dontpost,
	Post,
};

char	cputype[64];
char	sys[2*64];
char 	reply[256];
int	printcol;
int	mflag;
int	fflag;
int	kflag;
int	debugboot;

char	*bargv[Nbarg];
int	bargc;

static void	swapproc(void);
static Method	*rootserver(char*);
static void	kbmap(void);

/*
 * we should inherit the standard fds all referring to /dev/cons,
 * but we're being paranoid.
 */
static void
opencons(void)
{
	close(0);
	close(1);
	close(2);
	bind("#c", "/dev", MBEFORE);
	open("/dev/cons", OREAD);
	open("/dev/cons", OWRITE);
	open("/dev/cons", OWRITE);
}

/*
 * init will reinitialize its namespace.
 * #ec gets us plan9.ini settings (*var variables).
 */
static void
bindenvsrv(void)
{
	bind("#ec", "/env", MREPL);
	bind("#e", "/env", MBEFORE|MCREATE);
	bind("#s", "/srv/", MREPL|MCREATE);
}

static void
debuginit(int argc, char **argv)
{
	int fd;

	if(getenv("debugboot"))
		debugboot = 1;
#ifdef DEBUG
	print("argc=%d\n", argc);
	for(fd = 0; fd < argc; fd++)
		print("%#p %s ", argv[fd], argv[fd]);
	print("\n");
#endif	/* DEBUG */
	SET(fd, argc, argv); USED(fd, argc, argv);
}

/*
 * read disk partition tables here so that readnvram via factotum
 * can see them.  ideally we would have this information in
 * environment variables before attaching #S, which would then
 * parse them and create partitions.
 */
static void
partinit(void)
{
	char *rdparts;

	rdparts = getenv("readparts");
	if(rdparts)
		readparts();
	free(rdparts);
}

/*
 *  pick a method and initialize it
 */
static Method *
pickmethod(int argc, char **argv)
{
	Method *mp;

	if(method[0].name == nil)
		fatal("no boot methods");
	mp = rootserver(argc ? *argv : 0);
	(*mp->config)(mp);
	return mp;
}

/*
 *  authentication agent
 *  sets hostowner, creating an auth discontinuity
 */
static void
doauth(int cpuflag)
{
	if(debugboot)
		fprint(2, "auth...");
	authentication(cpuflag);
}

/*
 *  connect to the root file system
 */
static int
connectroot(Method *mp, int islocal, int ishybrid)
{
	int fd, n;
	char buf[32];

	fd = (*mp->connect)();
	if(fd < 0)
		fatal("can't connect to file server");
	if(getenv("srvold9p"))
		fd = old9p(fd);
	if(!islocal && !ishybrid){
		if(cfs)
			fd = (*cfs)(fd);
	}
	print("version...");
	buf[0] = '\0';
	n = fversion(fd, 0, buf, sizeof buf);
	if(n < 0)
		fatal("can't init 9P");
	srvcreate("boot", fd);
	return fd;
}

/*
 *  create the name space, mount the root fs
 */
static int
nsinit(int fd, char **rspp)
{
	int afd;
	char *rp, *rsp;
	AuthInfo *ai;
	static char rootbuf[64];

	if(bind("/", "/", MREPL) < 0)
		fatal("bind /");
	rp = getenv("rootspec");
	if(rp == nil)
		rp = "";
	
	afd = fauth(fd, rp);
	if(afd >= 0){
		ai = auth_proxy(afd, auth_getkey, "proto=p9any role=client");
		if(ai == nil)
			print("authentication failed (%r), trying mount anyways\n");
	}
	if(mount(fd, afd, "/root", MREPL|MCREATE, rp) < 0)
		fatal("mount /");
	rsp = rp;
	rp = getenv("rootdir");
	if(rp == nil)
		rp = rootdir;
	if(bind(rp, "/", MAFTER|MCREATE) < 0){
		if(strncmp(rp, "/root", 5) == 0){
			fprint(2, "boot: couldn't bind $rootdir=%s to root: %r\n", rp);
			fatal("second bind /");
		}
		snprint(rootbuf, sizeof rootbuf, "/root/%s", rp);
		rp = rootbuf;
		if(bind(rp, "/", MAFTER|MCREATE) < 0){
			fprint(2, "boot: couldn't bind $rootdir=%s to root: %r\n", rp);
			if(strcmp(rootbuf, "/root//plan9") != 0)
				fatal("second bind /");
			/* undo installer's work */
			fprint(2, "**** warning: remove rootdir=/plan9 "
				"entry from plan9.ini\n");
			rp = "/root";
			if(bind(rp, "/", MAFTER|MCREATE) < 0)
				fatal("second bind /");
		}
	}
	setenv("rootdir", rp);
	*rspp = rsp;
	return afd;
}

static void
execinit(void)
{
	int iargc;
	char *cmd, cmdbuf[64], *iargv[16];

	/* exec init */
	cmd = getenv("init");
	if(cmd == nil){
		sprint(cmdbuf, "/%s/init -%s%s", cputype,
			cpuflag ? "c" : "t", mflag ? "m" : "");
		cmd = cmdbuf;
	}
	iargc = tokenize(cmd, iargv, nelem(iargv)-1);
	cmd = iargv[0];

	/* make iargv[0] basename(iargv[0]) */
	if(iargv[0] = strrchr(iargv[0], '/'))
		iargv[0]++;
	else
		iargv[0] = cmd;

	iargv[iargc] = nil;

	chmod("/srv/" PARTSRV, 0600);
	exec(cmd, iargv);
	fatal(cmd);
}

void
boot(int argc, char *argv[])
{
	int fd, afd, islocal, ishybrid;
	char *rsp;
	Method *mp;

	fmtinstall('r', errfmt);
	opencons();
	bindenvsrv();
	debuginit(argc, argv);

	ARGBEGIN{
	case 'k':
		kflag = 1;
		break;
	case 'm':
		mflag = 1;
		break;
	case 'f':
		fflag = 1;
		break;
	}ARGEND

	readfile("#e/cputype", cputype, sizeof(cputype));

	/*
	 *  set up usb keyboard & mouse, if any.
	 *  starts partfs on first disk, if any, to permit nvram on usb.
	 */
	usbinit(Dontpost);

	mp = pickmethod(argc, argv);
	islocal = strcmp(mp->name, "local") == 0;
	ishybrid = strcmp(mp->name, "hybrid") == 0;

	kbmap();			/*  load keymap if it's there. */

	/* don't trigger aoe until the network has been configured */
	bind("#æ", "/dev", MAFTER);	/* nvram could be here */
	bind("#S", "/dev", MAFTER);	/* nvram could be here */
	partinit();

	doauth(cpuflag);	/* authentication usually changes hostowner */
	rfork(RFNAMEG);		/* leave existing subprocs in own namespace */
	usbinit(Post);		/* restart partfs under the new hostowner id */
	fd = connectroot(mp, islocal, ishybrid);
	afd = nsinit(fd, &rsp);
	close(fd);

	settime(islocal, afd, rsp);
	if(afd > 0)
		close(afd);
	swapproc();
	execinit();
	exits("failed to exec init");
}

static Method*
findmethod(char *a)
{
	Method *mp;
	int i, j;
	char *cp;

	if((i = strlen(a)) == 0)
		return nil;
	cp = strchr(a, '!');
	if(cp)
		i = cp - a;
	for(mp = method; mp->name; mp++){
		j = strlen(mp->name);
		if(j > i)
			j = i;
		if(strncmp(a, mp->name, j) == 0)
			break;
	}
	if(mp->name)
		return mp;
	return nil;
}

/*
 *  ask user from whence cometh the root file system
 */
static Method*
rootserver(char *arg)
{
	char prompt[256];
	Method *mp;
	char *cp;
	int n;

	/* look for required reply */
	readfile("#e/nobootprompt", reply, sizeof(reply));
	if(reply[0]){
		mp = findmethod(reply);
		if(mp)
			goto HaveMethod;
		print("boot method %s not found\n", reply);
		reply[0] = 0;
	}

	/* make list of methods */
	mp = method;
	n = sprint(prompt, "root is from (%s", mp->name);
	for(mp++; mp->name; mp++)
		n += sprint(prompt+n, ", %s", mp->name);
	sprint(prompt+n, ")");

	/* create default reply */
	readfile("#e/bootargs", reply, sizeof(reply));
	if(reply[0] == 0 && arg != 0)
		strcpy(reply, arg);
	if(reply[0]){
		mp = findmethod(reply);
		if(mp == 0)
			reply[0] = 0;
	}
	if(reply[0] == 0)
		strcpy(reply, method->name);

	/* parse replies */
	do{
		outin(prompt, reply, sizeof(reply));
		mp = findmethod(reply);
	}while(mp == nil);

HaveMethod:
	bargc = tokenize(reply, bargv, Nbarg-2);
	bargv[bargc] = nil;
	cp = strchr(reply, '!');
	if(cp)
		strcpy(sys, cp+1);
	return mp;
}

static void
swapproc(void)
{
	int fd;

	fd = open("#c/swap", OWRITE);
	if(fd < 0){
		warning("opening #c/swap");
		return;
	}
	if(write(fd, "start", 5) <= 0)
		warning("starting swap kproc");
	close(fd);
}

int
old9p(int fd)
{
	int p[2];

	if(pipe(p) < 0)
		fatal("pipe");

	print("srvold9p...");
	switch(fork()) {
	case -1:
		fatal("rfork srvold9p");
	case 0:
		dup(fd, 1);
		close(fd);
		dup(p[0], 0);
		close(p[0]);
		close(p[1]);
		execl("/srvold9p", "srvold9p", "-s", 0);
		fatal("exec srvold9p");
	default:
		close(fd);
		close(p[0]);
	}
	return p[1];
}

static void
kbmap(void)
{
	char *f;
	int n, in, out;
	char buf[1024];

	f = getenv("kbmap");
	if(f == nil)
		return;
	if(bind("#κ", "/dev", MAFTER) < 0){
		warning("can't bind #κ");
		return;
	}

	in = open(f, OREAD);
	if(in < 0){
		warning("can't open kbd map");
		return;
	}
	out = open("/dev/kbmap", OWRITE);
	if(out < 0) {
		warning("can't open /dev/kbmap");
		close(in);
		return;
	}
	while((n = read(in, buf, sizeof(buf))) > 0)
		if(write(out, buf, n) != n){
			warning("write to /dev/kbmap failed");
			break;
		}
	close(in);
	close(out);
}
