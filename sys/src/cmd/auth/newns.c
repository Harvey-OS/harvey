#include <u.h>
#include <libc.h>
#include <auth.h>

void
usage(void)
{
	fprint(2, "usage: newns [-f file] command\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *nsfile;

	nsfile = "/lib/namespace";
	ARGBEGIN{
	case 'f':
		nsfile = ARGF();
		break;
	default:
		usage();
		break;
	}ARGEND
	if(argc == 0)
		usage();
	newns(getuser(), nsfile);
	exec(argv[0], argv);
	sysfatal("exec: %s: %r", argv[0]);
}	
