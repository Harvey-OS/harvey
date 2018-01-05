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
#include <auth.h>
#include <fcall.h>
#include "../boot/boot.h"

char	cputype[64];
char	service[64];
char	sys[2*64];
int	printcol;
int	mflag;
int	fflag;
int	kflag;

char	*bargv[Nbarg];
int	bargc;

static Method	*rootserver(char*);
static void	usbinit(void);
static void	startconsole(void);
static void	kbmap(void);

void
boot(int argc, char *argv[])
{
	int fd, afd, srvt;
	Method *mp;
	char *cmd, cmdbuf[64], *iargv[16];
	char rootbuf[64];
	int islocal, ishybrid;
	char *rp, *rsp;
	int iargc, n;
	char buf[32];
	AuthInfo *ai;

	fmtinstall('r', errfmt);

	/*
	 *  start /dev/cons
	 */
	startconsole();

	/*
	 * init will reinitialize its namespace.
	 * #ec gets us plan9.ini settings (*var variables).
	 */
	bind("#ec", "/env", MREPL);
	bind("#e", "/env", MBEFORE|MCREATE);
	bind("#s", "/srv", MREPL|MCREATE);
	bind("#p", "/proc", MREPL|MCREATE);
	print("Hello, I am Harvey :-)\n");
#ifdef DEBUG
	print("argc=%d\n", argc);
	for(fd = 0; fd < argc; fd++)
		print("%#p %s ", argv[fd], argv[fd]);
	print("\n");
#endif //DEBUG

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
	readfile("#e/service", service, sizeof(service));

	/*
	 *  set up usb keyboard, mouse and disk, if any.
	 */
	usbinit();
	print("usbinit done\n");

	/*
	 *  pick a method and initialize it
	 */
	if(method[0].name == nil)
		fatal("no boot methods");
	mp = rootserver(argc ? *argv : 0);
	(*mp->config)(mp);
	islocal = strcmp(mp->name, "local") == 0;
	ishybrid = strcmp(mp->name, "hybrid") == 0;

	/*
	 *  load keymap if it is there.
	 */
	kbmap();

	/*
 	 *  authentication agent
	 */
	authentication(cpuflag);

print("connect...");
	/*
	 *  connect to the root file system
	 */
	fd = (*mp->connect)();
	if(fd < 0)
		fatal("can't connect to file server");
	if(!islocal && !ishybrid){
		if(cfs)
			fd = (*cfs)(fd);
	}
print("\n");
	print("version...");
	buf[0] = '\0';
	n = fversion(fd, 0, buf, sizeof buf);
	if(n < 0)
		fatal("can't init 9P");
	srvcreate("boot", fd);

	/*
	 *  create the name space, mount the root fs
	 */
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
	if(mount(fd, afd, "/root", MREPL|MCREATE, rp, 'M') < 0)
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
			if(strcmp(rootbuf, "/root//plan9") == 0){
				fprint(2, "**** warning: remove rootdir=/plan9 entry from plan9.ini\n");
				rp = "/root";
				if(bind(rp, "/", MAFTER|MCREATE) < 0)
					fatal("second bind /");
			}else
				fatal("second bind /");
		}
	}
	close(fd);
	setenv("rootdir", rp);

	settime(islocal, afd, rsp);
	if(afd > 0)
		close(afd);

	cmd = getenv("init");
	srvt = strcmp(service, "terminal");
	if(cmd == nil){
		if(srvt) {
			sprint(cmdbuf, "/%s/bin/init -%s%s", cputype,
				"t", mflag ? "m" : "");
			cmd = cmdbuf;
		} else {
			sprint(cmdbuf, "/%s/bin/init -%s%s", cputype,
				"c", mflag ? "m" : "");
			cmd = cmdbuf;
		}
	}
	iargc = tokenize(cmd, iargv, nelem(iargv)-1);
	cmd = iargv[0];

	/* make iargv[0] basename(iargv[0]) */
	if(iargv[0] = strrchr(iargv[0], '/'))
		iargv[0]++;
	else
		iargv[0] = cmd;

	iargv[iargc] = nil;

	exec(cmd, iargv);
	fatal(cmd);
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
	int rc;
	Method *mp;
	char *cp;
	char reply[256];
	int n;

	/* look for required reply */
	rc = readfile("#e/nobootprompt", reply, sizeof(reply));
	if(rc == 0 && reply[0]){
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
usbinit(void)
{
	static char usbd[] = "/boot/usbd";
	static char *argv[] = {"usbd"};

	if (access(usbd, AEXIST) < 0) {
		print("usbinit: no %s\n", usbd);
		return;
	}

	if(access("#u/usb/ctl", 0) < 0){
		print("usbinit: no #u/usb/ctl\n");
		return;
	}

	if (bind("#u", "/dev", MAFTER) < 0) {
		print("usbinit: can't bind #u to /dev: %r\n");
		return;
	}

	switch(fork()){
	case -1:
		print("usbinit: fork failed: %r\n");
	case 0:
		exec(usbd, argv);
		fatal("can't exec usbd");
	default:
		break;
	}
}

static void
startconsole(void)
{
	char *dbgfile, *argv[16], **av;
	int i;
	if(access("/boot/screenconsole", AEXEC) < 0)
		fatal("cannot access /boot/screenconsole");

	/* start agent */
	i = 0;
	av = argv;
	av[i++] = "screenconsole";
	if(dbgfile = getenv("debugconsole")){
		av[i++] = "-d";
		av[i++] = dbgfile;
	}
	av[i] = 0;
	switch(fork()){
	case -1:
		fatal("starting screenconsole");
	case 0:
		exec("/boot/screenconsole", av);
		fatal("execing /boot/screenconsole");
	default:
		break;
	}

	/* NOTE: this sleep and the access loop below are crap.
	 * this parent should wait on a pipe from screenconsole to get
	 * an 'OK' and then just go on to the open. Somebody should fix it.
	 * This nonsense is only needed on qemu because it is taking FOREVER
	 * to start processes on qemu, for reasons we don't understand.
	 * Note if we just sleep 250 it still fails sometimes.
	 * I think this is related to the weirdness in qemu where starting
	 * programs takes FOREVER.
	 */
	sleep(1000);
	/* wait for agent to really be there */
	while(access("#s/screenconsole", AEXIST) < 0){
		print("Screenconsole is STILL NOT READY\n");
		sleep(250);
	}
	/* replace 0, 1 and 2 */
	if((i = open("#s/screenconsole", ORDWR)) < 0)
		fatal("open #s/screenconsole");
	if(mount(i, -1, "/dev", MBEFORE, "", 'M') < 0)
		fatal("mount /dev");
	if((i = open("/dev/cons", OREAD))<0)
		fatal("open /dev/cons, OREAD");
	dup(i, 0);
	close(i);
	if((i = open("/dev/cons", OWRITE))<0)
		fatal("open /dev/cons, OWRITE");
	dup(i, 1);
	close(i);
	if(dup(1, 2) != 2)
		fatal("dup(1, 2)");
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
