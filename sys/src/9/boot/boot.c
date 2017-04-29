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
static void	kbmap(void);

static void acpiirq(void)
{
	static char *devs[] = {"#Z", "#$", "#P"};
	int i, pid, irqmap;
	Waitmsg *w;

	for (i = 0; i < nelem(devs); i++){
		if(bind(devs[i], "/dev", MAFTER) < 0){
			fprint(2, "Can't bind %s: %r", devs[i]);
			return;
		}
	}
	irqmap = open("/dev/irqmap", OREAD);
	if (irqmap < 0) {
		warning("can't open /dev/irqmap");
		return;
	}

	pid = fork();
	if (pid < 0){
		warning("Can't fork: %r");
		close(irqmap);
		return;
	}
	if (pid > 0) {
		close(irqmap);
		w = wait();
		if (w && w->msg && w->msg[0])
			warning(w->msg);
		return;
	}
	dup(irqmap, 0);
	close(irqmap);
	if (execl("/boot/irq", "irq", "-s", nil)) {
		print("note: can't start /boot/irq");
	}
	exits(nil);

}
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

	bind("#c", "/dev", MBEFORE);
	open("/dev/cons", OREAD);
	open("/dev/cons", OWRITE);
	open("/dev/cons", OWRITE);

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

	/* Do the initial ACPI interrupt setup work.
	 * If we don't do this we may not get needed
	 * interfaces. */
	if (getenv("acpiirq"))
		acpiirq();

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
		if(!srvt) {
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
	if((iargv[0] = strrchr(iargv[0], '/')) != nil)
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

	if (bind("#I", "/dev", MAFTER) < 0) {
		print("usbinit: can't bind #I to /dev: %r\n");
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
