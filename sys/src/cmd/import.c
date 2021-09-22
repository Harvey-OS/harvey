/*
 * import a namespace from a remote system
 */
#include <u.h>
#include <libc.h>
#include <auth.h>
#include <libsec.h>

enum {
	Encunspec,
	Encnone,
	Encssl,
	Enctls,
};

static char *encprotos[] = {
	[Encunspec] =	"unspec",	/* unspecified; not sent over the net */
	[Encnone] =	"clear",
	[Encssl] =	"ssl",
	[Enctls] = 	"tls",
			nil,
};

char		*keyspec = "";
char		*filterp;
char		*ealgs = "rc4_256 sha1";	/* ssl only */
int		encproto = Encunspec;
char		*aan = "/bin/aan";
AuthInfo 	*ai;
int		debug;
int		doauth = 1;
int		timedout;

void	catcher(void*, char*);
int	connect(char*, char*);
int	filter(int, char *, char *);
int	passive(void);
void	sysfatal(char*, ...);
void	usage(void);

static void	mksecret(char *, uchar *);

/*
 * based on libthread's threadsetname, but drags in less library code.
 * actually just sets the arguments displayed.
 */
void
procsetname(char *fmt, ...)
{
	int fd;
	char *cmdname;
	char buf[128];
	va_list arg;

	va_start(arg, fmt);
	cmdname = vsmprint(fmt, arg);
	va_end(arg);
	if (cmdname == nil)
		return;
	snprint(buf, sizeof buf, "#p/%d/args", getpid());
	if((fd = open(buf, OWRITE)) >= 0){
		write(fd, cmdname, strlen(cmdname)+1);
		close(fd);
	}
	free(cmdname);
}

void
post(char *name, char *envname, int srvfd)
{
	int fd;
	char buf[32];

	fd = create(name, OWRITE, 0600);
	if(fd < 0)
		return;
	sprint(buf, "%d",srvfd);
	if(write(fd, buf, strlen(buf)) != strlen(buf))
		sysfatal("srv write: %r");
	close(fd);
	putenv(envname, name);
}

static int
lookup(char *s, char *l[])
{
	int i;

	for (i = 0; l[i] != 0; i++)
		if (strcmp(l[i], s) == 0)
			return i;
	return -1;
}

static int
pushtlsclient(int fd)
{
	int efd;
	TLSconn conn;

	memset(&conn, 0, sizeof conn);
	efd = tlsClient(fd, &conn);
	free(conn.cert);
	return efd;
}

static int
tryenc(int fd, char *host, int encproto, AuthInfo *ai)
{
	uchar key[16], digest[SHA1dlen];
	char fromclientsecret[21], fromserversecret[21];
	int i, tls;

	memmove(key+4, ai->secret, ai->nsecret);

	/* exchange random numbers */
	srand(truerand());
	for(i = 0; i < 4; i++)
		key[i] = rand();
	if(write(fd, key, 4) != 4)
		sysfatal("can't write key part: %r");
	if(readn(fd, key+12, 4) != 4) {
		werrstr("can't read key part: %r");
		close(fd);
		return -1;
	}

	/* scramble into two secrets */
	sha1(key, sizeof(key), digest, nil);
	mksecret(fromclientsecret, digest);
	mksecret(fromserversecret, digest+10);

	if (filterp)
		fd = filter(fd, filterp, host);

	/* set up encryption */
	assert(encproto != Encunspec);
	tls = encproto == Enctls;
	procsetname("push%s client", tls? "tls": "ssl");
	if (tls)
		fd = pushtlsclient(fd);
	else
		fd = pushssl(fd, ealgs, fromclientsecret, fromserversecret, nil);
	if(fd < 0)
		werrstr("can't establish %s connection: %r", tls? "tls": "ssl");
	return fd;
}

/*
 * if fd < 0, connect and set fd.  optionally add encryption and aan to fd.
 */
static int
connectenc(int fd, int encproto, char *host, char *tree)
{
	if (fd < 0)
		fd = connect(host, tree);
	if (fd < 0)
		return fd;

	assert(encproto != Encunspec);
	fprint(fd, "impo %s %s\n", filterp? "aan": "nofilter",
		encprotos[encproto]);
	if (encproto != Encnone && encproto != Encunspec && ealgs && ai)
		fd = tryenc(fd, host, encproto, ai);
	else if (filterp)
		fd = filter(fd, filterp, host);
	return fd;
}

/*
 * post fd and mount it.
 */
static int
postmnt(int fd, char *host, char *tree, char *mntpt, int mntflags, char *srvpost)
{
	char srvfile[64];

	if(srvpost){
		sprint(srvfile, "/srv/%s", srvpost);
		remove(srvfile);
		post(srvfile, srvpost, fd);
	}

	procsetname("mount on %s", mntpt);
	if(mount(fd, -1, mntpt, mntflags, "") < 0)
		sysfatal("can't mount %s's %s on %s: %r", host, tree, mntpt);

	assert(encproto != Encunspec);
	if (encproto != Enctls)
		fprint(2, "%s: caution: using %s encryption, not tls, for %s's %s\n",
			argv0, encprotos[encproto], host, tree);
	return 0;
}

void
main(int argc, char **argv)
{
	char *mntpt, *srvpost, *host, *tree;
	int i, backwards = 0, fd, mntflags, notree;

	quotefmtinstall();
	srvpost = nil;
	notree = 0;
	mntflags = MREPL;
	ARGBEGIN{
	case 'A':
		doauth = 0;
		break;
	case 'a':
		mntflags = MAFTER;
		break;
	case 'B':
		backwards = 1;
		break;
	case 'b':
		mntflags = MBEFORE;
		break;
	case 'c':
		mntflags |= MCREATE;
		break;
	case 'C':
		mntflags |= MCACHE;
		break;
	case 'd':
		debug++;
		break;
	case 'E':
		if ((encproto = lookup(EARGF(usage()), encprotos)) < 0)
			usage();
		break;
	case 'e':
		ealgs = EARGF(usage());
		if(*ealgs == 0 || strcmp(ealgs, "clear") == 0)
			ealgs = nil;
		break;
	case 'f':
		/* ignored but allowed for compatibility */
		break;
	case 'k':
		keyspec = EARGF(usage());
		break;
	case 'm':
		notree = 1;
		break;
	case 'p':
		filterp = aan;
		break;
	case 's':
		srvpost = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	mntpt = 0;		/* to shut up compiler */
	tree = nil;
	if(backwards){
		if (argc > 0)
			mntpt = argv[0];
		else
			usage();
	} else {
		switch(argc) {
		case 2:
			mntpt = argv[1];
			break;
		case 3:
			if(notree)
				usage();
			mntpt = argv[2];
			break;
		default:
			usage();
		}
	}
	if (!notree)
		tree = argv[1];

	notify(catcher);
	alarm(60*1000);

	host = argv[0];
	if(backwards) {
		if (encproto == Encunspec)
			sysfatal("must specify encryption: tls, ssl or clear");
		fd = connectenc(passive(), encproto, host, tree);
		if (fd < 0)
			sysfatal("%r");
		postmnt(fd, host, tree, mntpt, mntflags, srvpost);
		if(argc > 1){
			alarm(0);
			exec(argv[1], &argv[1]);
			sysfatal("exec: %r");
		}
		exits(0);
	}

	/* normal forward cases */
	if (encproto == Encunspec) {
		/* try tls, ssl and finally none */
		static int tryencs[] = { Enctls, Encssl, Encnone };

		fd = -1;
		for (i = 0; fd < 0 && i < nelem(tryencs); i++) {
			encproto = tryencs[i];
			fd = connectenc(-1, encproto, host, tree);
		}
	} else
		fd = connectenc(-1, encproto, host, tree);
	if (fd < 0)
		sysfatal("%r");
	alarm(0);
	postmnt(fd, host, tree, mntpt, mntflags, srvpost);
	exits(0);
}

void
catcher(void*, char *msg)
{
	timedout = 1;
	if(strcmp(msg, "alarm") == 0)
		noted(NCONT);
	noted(NDFLT);
}

int
connect(char *system, char *tree)
{
	char buf[ERRMAX], dir[128], *na;
	int fd, n, oalarm;
	char *authp;

	na = netmkaddr(system, 0, "exportfs");
	procsetname("dial %s", na);
	oalarm = alarm(30*1000);		/* adjust to taste */
	fd = dial(na, 0, dir, 0);
	alarm(oalarm);
	if(fd < 0) {
		werrstr("can't dial %s: %r", system);
		return -1;
	}

	if(doauth){
		authp = "p9any";
		procsetname("auth_proxy auth_getkey proto=%q role=client %s",
			authp, keyspec);
		ai = auth_proxy(fd, auth_getkey, "proto=%q role=client %s",
			authp, keyspec);
		if(ai == nil)
			sysfatal("%r: %s", system);
	}

	if(tree != nil){
		procsetname("writing tree name %s", tree);
		n = write(fd, tree, strlen(tree));
		if(n < 0)
			sysfatal("can't write tree: %r");

		strcpy(buf, "can't read tree");

		procsetname("awaiting OK for %s", tree);
		n = read(fd, buf, sizeof buf - 1);
		if(n!=2 || buf[0]!='O' || buf[1]!='K'){
			if (timedout)
				sysfatal("timed out connecting to %s", na);
			buf[sizeof buf - 1] = '\0';
			sysfatal("bad remote tree: %s", buf);
		}
	}
	return fd;
}

int
passive(void)
{
	int fd;

	/*
	 * Ignore doauth==0 on purpose.  Is it useful here?
	 */

	procsetname("auth_proxy auth_getkey proto=p9any role=server");
	ai = auth_proxy(0, auth_getkey, "proto=p9any role=server");
	if(ai == nil)
		sysfatal("auth_proxy: %r");
	if(auth_chuid(ai, nil) < 0)
		sysfatal("auth_chuid: %r");
	putenv("service", "import");

	fd = dup(0, -1);
	close(0);
	open("/dev/null", ORDWR);
	close(1);
	open("/dev/null", ORDWR);

	return fd;
}

void
usage(void)
{
	fprint(2, "usage: import [-abcCm] [-A] [-E unspec|clear|ssl|tls] "
"[-e 'crypt auth'|clear] [-k keypattern] [-p] host remotefs [mountpoint]\n");
	exits("usage");
}

/* Network on fd1, mount driver on fd0 */
int
filter(int fd, char *cmd, char *host)
{
	int p[2], len, argc;
	char newport[256], buf[256], *s;
	char *argv[16], *file, *pbuf;

	if ((len = read(fd, newport, sizeof newport - 1)) < 0)
		sysfatal("filter: cannot write port; %r");
	newport[len] = '\0';

	if ((s = strchr(newport, '!')) == nil)
		sysfatal("filter: illegally formatted port %s", newport);

	strecpy(buf, buf+sizeof buf, netmkaddr(host, "tcp", "0"));
	pbuf = strrchr(buf, '!');
	strecpy(pbuf, buf+sizeof buf, s);

	if(debug)
		fprint(2, "filter: remote port %s\n", newport);

	argc = tokenize(cmd, argv, nelem(argv)-2);
	if (argc == 0)
		sysfatal("filter: empty command");
	argv[argc++] = "-c";
	argv[argc++] = buf;
	argv[argc] = nil;
	file = argv[0];
	if (s = strrchr(argv[0], '/'))
		argv[0] = s+1;

	if(pipe(p) < 0)
		sysfatal("pipe: %r");

	switch(rfork(RFNOWAIT|RFPROC|RFFDG)) {
	case -1:
		sysfatal("rfork record module: %r");
	case 0:
		dup(p[0], 1);
		dup(p[0], 0);
		close(p[0]);
		close(p[1]);
		exec(file, argv);
		sysfatal("exec record module: %r");
	default:
		close(fd);
		close(p[0]);
	}
	return p[1];
}

static void
mksecret(char *t, uchar *f)
{
	sprint(t, "%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux",
		f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9]);
}
