#include <u.h>
#include <libc.h>
#include <auth.h>

void
usage(void)
{
	fprint(2, "usage: newns [-n namespace] [cmd [args...]]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *nsfile;
	char *defargv[] = { "/bin/rc", "-i", nil };

	nsfile = "/lib/namespace";
	ARGBEGIN{
	case 'n':
		nsfile = ARGF();
		break;
	default:
		usage();
		break;
	}ARGEND
	if(argc == 0)
		argv = defargv;
	newns(getuser(), nsfile);
	exec(argv[0], argv);
	sysfatal("exec: %s: %r", argv[0]);
}	
