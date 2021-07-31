#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>

int	connect(char*, char*);
int	filter(int);
int	fflag;
void	catcher(void*, char*);
void	error(char*, ...);
void	usage(void);

void
main(int argc, char **argv)
{
	char *mntpt;
	int fd, mntflags;

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
	case 'f':
		fflag = 1;
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

	notify(catcher);
	alarm(60*1000);
	fd = connect(argv[0], argv[1]);
	if(fflag)
		fd = filter(fd);

	if(amount(fd, mntpt, mntflags, "") < 0)
		error("can't mount %s: %r", argv[1]);
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
connect(char *system, char *tree)
{
	char buf[ERRLEN], dir[4*NAMELEN], *na;
	int fd, n;

	na = netmkaddr(system, 0, "exportfs");
	if((fd = dial(na, 0, dir, 0)) < 0)
		error("can't dial %s: %r", system);

	if(auth(fd) < 0)
		error("%r: %s", system);

	if(strstr(dir, "tcp"))
		fflag = 1;

	n = write(fd, tree, strlen(tree));
	if(n < 0)
		error("can't write tree: %r");

	strcpy(buf, "can't read tree");

	n = read(fd, buf, sizeof buf - 1);
	if(n!=2 || buf[0]!='O' || buf[1]!='K'){
		buf[sizeof buf - 1] = '\0';
		error("bad remote tree: %s\n", buf);
	}
	return fd;
}

void
error(char *fmt, ...)
{
	char msg[256];
	va_list arg;

	va_start(arg, fmt);
	doprint(msg, msg+sizeof(msg), fmt, arg);
	va_end(arg);
	fprint(2, "import: %s\n", msg);
	exits(msg);
}

void
usage(void)
{
	print("Usage: import [-abcC] host remotefs [mountpoint]\n");
	exits("usage");
}

void
fatal(int syserr, char *fmt, ...)
{
	char buf[ERRLEN];
	va_list arg;

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	if(syserr)
		fprint(2, "cpu: %s: %r\n", buf);
	else
		fprint(2, "cpu: %s\n", buf);
	exits(buf);
}

/* Network on fd1, mount driver on fd0 */
int
filter(int fd)
{
	int p[2];

	if(pipe(p) < 0)
		fatal(1, "pipe");

	switch(rfork(RFNOWAIT|RFPROC|RFFDG)) {
	case -1:
		fatal(1, "rfork record module");
	case 0:
		dup(fd, 1);
		close(fd);
		dup(p[0], 0);
		close(p[0]);
		close(p[1]);
		execl("/bin/aux/fcall", "fcall", 0);
		fatal(1, "exec record module");
	default:
		close(fd);
		close(p[0]);
	}
	return p[1];	
}
