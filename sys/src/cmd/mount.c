#include <u.h>
#include <libc.h>
#include <auth.h>

void	usage(void);
void	catch(void*, char*);

void
main(int argc, char *argv[])
{
	char *spec;
	ulong flag = 0;
	int qflag = 0;
	int fd;

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
	case 'q':
		qflag = 1;
		break;
	default:
		usage();
	}ARGEND

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
	if(amount(fd, argv[1], flag, spec) < 0){
		if(qflag)
			exits(0);
		fprint(2, "%s: mount %s: %r\n", argv0, argv[1]);
		exits("mount");
	}
	exits(0);
}

void
catch(void *x, char *m)
{
	USED(x);
	fprint(2, "mount: %s\n", m);
	exits(m);
}

void
usage(void)
{
	fprint(2, "usage: mount [-a|-b] [-c] [-r] /srv/service dir [spec]\n");
	exits("usage");
}
