#include <u.h>
#include <libc.h>
#include <auth.h>
#include <libsec.h>

enum {
	Encnone,
	Encssl,
	Enctls,
};

static char *encprotos[] = {
	[Encnone] =	"clear",
	[Encssl] =		"ssl",
	[Enctls] = 		"tls",
				nil,
};

char		*filterp;
char		*ealgs = "rc4_256 sha1";
int		encproto = Encnone;
char		*aan = "/bin/aan";
AuthInfo 	*ai;
int		debug;

int	connect(char*, char*, int);
int	old9p(int);
void	catcher(void*, char*);
void	sysfatal(char*, ...);
void	usage(void);
int	filter(int, char *, char *);

static void	mksecret(char *, uchar *);

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

void
main(int argc, char **argv)
{
	char *mntpt;
	int fd, mntflags;
	int oldserver;
	char *srvpost, srvfile[64];

	srvpost = nil;
	oldserver = 0;
	mntflags = MREPL;
	ARGBEGIN{
	case 'a':
		mntflags = MAFTER;
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
	case 'f':
		/* ignored but allowed for compatibility */
		break;
	case 'O':
	case 'o':
		oldserver = 1;
		break;
	case 'E':
		if ((encproto = lookup(EARGF(usage()), encprotos)) < 0)
			usage();
		break;
	case 'e':
		ealgs = ARGF();
		if(ealgs == nil)
			usage();
		if(*ealgs == 0 || strcmp(ealgs, "clear") == 0)
			ealgs = nil;
		break;

	case 'p':
		filterp = aan;
		break;
	case 's':
		srvpost = ARGF();
		break;
	default:
		usage();
	}ARGEND;

	switch(argc) {
	case 2:
		mntpt = argv[1];
		break;
	case 3:
		mntpt = argv[2];
		break;		
	default:
		mntpt = 0;		/* to shut up compiler */
		usage();
	}

	if (encproto == Enctls)
		sysfatal("%s: tls has not yet been implemented\n", argv[0]);

	notify(catcher);
	alarm(60*1000);
	fd = connect(argv[0], argv[1], oldserver);

	if (!oldserver)
		fprint(fd, "impo %s %s\n", filterp? "aan": "nofilter", encprotos[encproto]);

	if (encproto != Encnone && ealgs && ai) {
		uchar key[16];
		uchar digest[SHA1dlen];
		char fromclientsecret[21];
		char fromserversecret[21];
		int i;

		memmove(key+4, ai->secret, ai->nsecret);

		/* exchange random numbers */
		srand(truerand());
		for(i = 0; i < 4; i++)
			key[i] = rand();
		if(write(fd, key, 4) != 4)
			sysfatal("can't write key part: %r");
		if(readn(fd, key+12, 4) != 4)
			sysfatal("can't read key part: %r");

		/* scramble into two secrets */
		sha1(key, sizeof(key), digest, nil);
		mksecret(fromclientsecret, digest);
		mksecret(fromserversecret, digest+10);

		if (filterp)
			fd = filter(fd, filterp, argv[0]);

		/* set up encryption */
		fd = pushssl(fd, ealgs, fromclientsecret, fromserversecret, nil);
		if(fd < 0)
			sysfatal("can't establish ssl connection: %r");
	}
	else if (filterp) 
		fd = filter(fd, filterp, argv[0]);

	if(srvpost){
		sprint(srvfile, "/srv/%s", srvpost);
		remove(srvfile);
		post(srvfile, srvpost, fd);
	}
	if(mount(fd, -1, mntpt, mntflags, "") < 0)
		sysfatal("can't mount %s: %r", argv[1]);
	alarm(0);
	exits(0);
}

void
catcher(void*, char *msg)
{
	if(strcmp(msg, "alarm") == 0)
		noted(NCONT);
	noted(NDFLT);
}

int
old9p(int fd)
{
	int p[2];

	if(pipe(p) < 0)
		sysfatal("pipe: %r");

	switch(rfork(RFPROC|RFFDG|RFNAMEG)) {
	case -1:
		sysfatal("rfork srvold9p: %r");
	case 0:
		if(fd != 1){
			dup(fd, 1);
			close(fd);
		}
		if(p[0] != 0){
			dup(p[0], 0);
			close(p[0]);
		}
		close(p[1]);
		if(0){
			fd = open("/sys/log/cpu", OWRITE);
			if(fd != 2){
				dup(fd, 2);
				close(fd);
			}
			execl("/bin/srvold9p", "srvold9p", "-ds", 0);
		} else
			execl("/bin/srvold9p", "srvold9p", "-s", 0);
		sysfatal("exec srvold9p: %r");
	default:
		close(fd);
		close(p[0]);
	}
	return p[1];	
}

int
connect(char *system, char *tree, int oldserver)
{
	char buf[ERRMAX], dir[128], *na;
	int fd, n;
	char *authp;

	na = netmkaddr(system, 0, "exportfs");
	if((fd = dial(na, 0, dir, 0)) < 0)
		sysfatal("can't dial %s: %r", system);

	if(oldserver)
		authp = "p9sk2";
	else
		authp = "p9any";

	ai = auth_proxy(fd, auth_getkey, "proto=%q role=client", authp);
	if(ai == nil)
		sysfatal("%r: %s", system);

	n = write(fd, tree, strlen(tree));
	if(n < 0)
		sysfatal("can't write tree: %r");

	strcpy(buf, "can't read tree");

	n = read(fd, buf, sizeof buf - 1);
	if(n!=2 || buf[0]!='O' || buf[1]!='K'){
		buf[sizeof buf - 1] = '\0';
		sysfatal("bad remote tree: %s", buf);
	}

	if(oldserver)
		return old9p(fd);
	return fd;
}

void
usage(void)
{
	print("Usage: import [-abcC] [-E clear|ssl|tls] [-e 'crypt auth'|clear] [-p] host remotefs [mountpoint]\n");
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
		sysfatal("filter: cannot write port; %r\n");
	newport[len] = '\0';

	if ((s = strchr(newport, '!')) == nil)
		sysfatal("filter: illegally formatted port %s\n", newport);

	strcpy(buf, netmkaddr(host, "il", "0"));
	pbuf = strrchr(buf, '!');
	strcpy(pbuf, s);

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

