#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <auth.h>

int	connect(char*, char*);
void	error(int, char*, ...);
void	usage(void);

void
main(int argc, char **argv)
{
	char *mntpt;
	int fd, mntflags;
	char *srv;

	srv = "";
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
	case 't':
		srv = "any";
		break;
	case 's':
		srv = ARGF();
		if(srv == 0)
			usage();
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

	fd = connect(argv[0], argv[1]);
	if(mount(fd, mntpt, mntflags, "", srv) < 0)
		error(1, "can't mount %s", argv[1]);
	exits(0);
}

int
connect(char *system, char *tree)
{
	char buf[ERRLEN], *na, *err;
	int fd, n;

	na = netmkaddr(system, 0, "exportfs");
	if((fd = dial(na, 0, 0, 0)) < 0)
		error(1, "can't dial %s", system);

	if(err = auth(fd, na))
		error(1, "%s: %s", err, system);

	n = write(fd, tree, strlen(tree));
	if(n < 0)
		error(1, "can't write tree");

	strcpy(buf, "can't read tree");

	n = read(fd, buf, sizeof buf - 1);
	if(n!=2 || buf[0]!='O' || buf[1]!='K'){
		buf[sizeof buf - 1] = '\0';
		error(0, "bad remote tree: %s\n", buf);
	}
	return fd;
}

void
error(int syserr, char *fmt, ...)
{
	char msg[256];

	doprint(msg, msg+sizeof(msg), fmt, (&fmt+1));
	if(syserr)
		fprint(2, "import: %s: %r\n", msg);
	else
		fprint(2, "import: %s\n", msg);
	exits(msg);
}

void
usage(void)
{
	print("Usage: import [-abc] [-t|-s server] host remotefs [mountpoint]\n");
	exits("usage");
}
