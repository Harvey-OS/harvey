#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "../boot/boot.h"

char	cputype[NAMELEN];
char	sys[2*NAMELEN];
char	username[NAMELEN];
char 	reply[256];
int	printcol;
int	mflag;
int	fflag;
int	kflag;

char	*bargv[Nbarg];
int	bargc;

static void	swapproc(void);
static void	recover(Method*);
static Method	*rootserver(char*);

static int
rconv(va_list *arg, Fconv *fp)
{
	char s[ERRLEN];

	USED(arg);

	s[0] = 0;
	errstr(s);
	strconv(s, fp);
	return 0;
}

void
boot(int argc, char *argv[])
{
	int fd;
	Method *mp;
	char *cmd, cmdbuf[64], *iargv[16];
	char rootbuf[64];
	int islocal, ishybrid, iargc;
	char *rp;

	sleep(1000);

	fmtinstall('r', rconv);

	open("#c/cons", OREAD);
	open("#c/cons", OWRITE);
	open("#c/cons", OWRITE);
	bind("#c", "/dev", MAFTER);
	bind("#e", "/env", MREPL|MCREATE);

#ifdef DEBUG
	print("argc=%d\n", argc);
	for(fd = 0; fd < argc; fd++)
		print("%lux %s ", argv[fd], argv[fd]);
	print("\n");
#endif DEBUG

	ARGBEGIN{
	case 'u':
		strcpy(username, ARGF());
		break;
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
	 *  pick a method and initialize it
	 */
	mp = rootserver(argc ? *argv : 0);
	(*mp->config)(mp);
	islocal = strcmp(mp->name, "local") == 0;
	ishybrid = strcmp(mp->name, "hybrid") == 0;

	/*
	 *  get/set key or password
	 */
	(*pword)(islocal, mp);

	switch(rfork(RFPROC|RFNAMEG|RFFDG)) {
	case -1:
		print("failed to start recover: %r\n");
		break;
	case 0:
		recover(mp);
		break;
	}

	/*
	 *  connect to the root file system
	 */
	fd = (*mp->connect)();
	if(fd < 0)
		fatal("can't connect to file server");
	nop(fd);
	if(!islocal && !ishybrid){
		if(cfs)
			fd = (*cfs)(fd);
		doauthenticate(fd, mp);
	}
	srvcreate("boot", fd);

	/*
	 *  create the name space, mount the root fs
	 */
	if(bind("/", "/", MREPL) < 0)
		fatal("bind /");
	rp = getenv("rootspec");
	if(rp == nil)
		rp = "";
	if(mount(fd, "/root", MREPL|MCREATE, rp) < 0)
		fatal("mount /");
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

	/*
	 *  if a local file server exists and it's not
	 *  running, start it and mount it onto /n/kfs
	 */
	if(access("#s/kfs", 0) < 0){
		for(mp = method; mp->name; mp++){
			if(strcmp(mp->name, "local") != 0)
				continue;
			bargc = 1;
			(*mp->config)(mp);
			fd = (*mp->connect)();
			if(fd < 0)
				break;
			mount(fd, "/n/kfs", MAFTER|MCREATE, "") ;
			close(fd);
			break;
		}
	}

	settime(islocal);
	swapproc();

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

	exec(cmd, iargv);
	fatal(cmd);
}

Method*
findmethod(char *a)
{
	Method *mp;
	int i, j;
	char *cp;

	i = strlen(a);
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
	return 0;
}

int
parsecmd(char *s, char **av, int n)
{
	int ac;

	n--;
	for(ac = 0; ac < n; ac++){
		while(*s == ' ' || *s == '\t')
			s++;
		if(*s == 0 || *s == '\n' || *s == '\r')
			break;
		if(*s == '\''){
			s++;
			av[ac] = s;
			while(*s && *s != '\'')
				s++;
		} else {
			av[ac] = s;
			while(*s && *s != ' ' && *s != '\t')
				s++;
		}
		if(*s != 0)
			*s++ = 0;
	}
	av[ac] = 0;
	return ac;
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
	for(;;){
		outin(prompt, reply, sizeof(reply));
		mp = findmethod(reply);
		if(mp){
			bargc = parsecmd(reply, bargv, Nbarg);
			cp = strchr(reply, '!');
			if(cp)
				strcpy(sys, cp+1);
			return mp;
		}
	}

	return 0;		/* not reached */
}

int
nop(int fd)
{
	int n;
	Fcall hdr;
	char buf[128];

	print("boot: nop...");
	hdr.type = Tnop;
	hdr.tag = NOTAG;
	n = convS2M(&hdr, buf);
	if(write(fd, buf, n) != n){
		fatal("write nop");
		return 0;
	}
reread:
	n = read(fd, buf, sizeof buf);
	if(n <= 0){
		fatal("read nop");
		return 0;
	}
	if(n == 2)
		goto reread;
	if(convM2S(buf, &hdr, n) == 0) {
		fatal("format nop");
		return 0;
	}
	if(hdr.type != Rnop){
		fatal("not Rnop");
		return 0;
	}
	if(hdr.tag != NOTAG){
		fatal("tag not NOTAG");
		return 0;
	}
	return 1;
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

void
reattach(int rec, Method *amp, char *buf)
{
	char *mp;
	int fd, n, sv[2];
	char tmp[64], *p;

	mp = strchr(buf, ' ');
	if(mp == 0)
		goto fail;
	*mp++ = '\0';

	p = strrchr(buf, '/');
	if(p == 0)
		goto fail;

	*p = '\0';

	sprint(tmp, "%s/remote", buf);
	fd = open(tmp, OREAD);
	if(fd < 0)
		goto fail;

	n = read(fd, tmp, sizeof(tmp));
	if(n < 0)
		goto fail;

	close(fd);
	tmp[n-1] = '\0';

	print("boot: Service %s!%s down, wait...\n", buf, tmp);

	p = strrchr(buf, '/');
	if(p == 0)
		goto fail;
	*p = '\0';

	while(plumb(buf, tmp, sv, 0) < 0)
		sleep(30);

	nop(sv[1]);
	doauthenticate(sv[1], amp);

	print("\nboot: Service %s Ok\n", tmp);

	n = sprint(tmp, "%d %s", sv[1], mp);
	if(write(rec, tmp, n) < 0) {
		errstr(tmp);
		print("write recover: %s\n", tmp);
	}
	exits(0);
fail:
	print("recover fail: %s\n", buf);
	exits(0);
}

static void
recover(Method *mp)
{
	int fd, n;
	char buf[256];

	fd = open("#/./recover", ORDWR);
	if(fd < 0)
		exits(0);

	for(;;) {
		n = read(fd, buf, sizeof(buf));
		if(n < 0)
			exits(0);
		buf[n] = '\0';

		if(fork() == 0)
			reattach(fd, mp, buf);
	}
}
