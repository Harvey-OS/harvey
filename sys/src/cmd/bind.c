#include <u.h>
#include <libc.h>

void	usage(void);

void
main(int argc, char *argv[])
{
	ulong flag = 0;
	char buf[ERRLEN];

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
	default:
		usage();
	}ARGEND

	if(argc != 2 || (flag&MAFTER)&&(flag&MBEFORE))
		usage();

	if(bind(argv[0], argv[1], flag) < 0){
		errstr(buf);
		fprint(2, "bind %s %s: %s\n", argv[0], argv[1], buf);
		exits("bind");
	}
	exits(0);
}

void
usage(void)
{
	fprint(2, "usage: bind [-b|-a|-c|-bc|-ac] new old\n");
	exits("usage");
}
