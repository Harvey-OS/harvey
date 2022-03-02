#include <u.h>
#include <libc.h>
#include <auth.h>

void	usage(void);
void	catch(void*, char*);

char *keyspec = "";

int
amount0(int fd, char *mntpt, int flags, char *aname, char *keyspec, int dev)
{
	int rv, afd;
	AuthInfo *ai;

	afd = fauth(fd, aname);
	if(afd >= 0){
		ai = auth_proxy(afd, amount_getkey, "proto=p9any role=client %s", keyspec);
		if(ai != nil)
			auth_freeAI(ai);
		else
			fprint(2, "%s: auth_proxy: %r\n", argv0);
	}
	rv = nmount(fd, afd, mntpt, flags, aname, dev);
	if(afd >= 0)
		close(afd);
	return rv;
}

void
main(int argc, char *argv[])
{
	char *spec;
	ulong flag = 0;
	int qflag = 0;
	int noauth = 0;
	char *mountname = "M";
	int fd, rv;

	ARGBEGIN{
	case 'a':
		flag |= MAFTER;
		break;
	case 'b':
		flag |= MBEFORE;
		break;
	case 'c':
		flag |= MCREATE;
		break;
	case 'C':
		flag |= MCACHE;
		break;
	case 'k':
		keyspec = EARGF(usage());
		break;
	case 'M':
		mountname = EARGF(usage());
		break;
	case 'n':
		noauth = 1;
		break;
	case 'q':
		qflag = 1;
		break;
	default:
		usage();
	}ARGEND

	if (strlen(mountname) != 1) {
		exits("mountname (-M) must be 1 char");
	}

	spec = 0;
	if(argc == 2)
		spec = "";
	else if(argc == 3)
		spec = argv[2];
	else
		usage();

	if((flag&MAFTER)&&(flag&MBEFORE))
		usage();

	fd = open(argv[0], ORDWR);
	if(fd < 0){
		if(qflag)
			exits(0);
		fprint(2, "%s: can't open %s: %r\n", argv0, argv[0]);
		exits("open");
	}

	notify(catch);
	if(noauth)
		rv = nmount(fd, -1, argv[1], flag, spec, (int)mountname[0]);
	else
		rv = amount0(fd, argv[1], flag, spec, keyspec, (int)mountname[0]);
	if(rv < 0){
		if(qflag)
			exits(0);
		fprint(2, "%s: nmount %s: %r\n", argv0, argv[1]);
		exits("nmount");
	}
	exits(0);
}

void
catch(void *x, char *m)
{
	USED(x);
	fprint(2, "nmount: %s\n", m);
	exits(m);
}

void
usage(void)
{
	fprint(2, "usage: nmount [-a|-b] [-cnq] [-k keypattern] [-M dc-specifier (one UTF-8 character)] /srv/service dir [spec]\n");
	exits("usage");
}
