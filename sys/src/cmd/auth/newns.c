#include <u.h>
#include <libc.h>
#include <auth.h>

void
usage(void)
{
	fprint(2, "usage: newns [-ad] [-n namespace] [cmd [args...]]\n");
	exits("usage");
}

static int
rooted(char *s)
{
	if(s[0] == '/')
		return 1;
	if(s[0] == '.' && s[1] == '/')
		return 1;
	if(s[0] == '.' && s[1] == '.' && s[2] == '/')
		return 1;
	return 0;
}

void
main(int argc, char **argv)
{
	extern int newnsdebug;
	char *defargv[] = { "/bin/rc", "-i", nil };
	char *nsfile, err[ERRMAX];
	int add;

	rfork(RFNAMEG);
	add = 0;
	nsfile = "/lib/namespace";
	ARGBEGIN{
	case 'a':
		add = 1;
		break;
	case 'd':
		newnsdebug = 1;
		break;
	case 'n':
		nsfile = ARGF();
		break;
	default:
		usage();
		break;
	}ARGEND
	if(argc == 0)
		argv = defargv;
	if (add)
		addns(getuser(), nsfile);
	else
		newns(getuser(), nsfile);
	exec(argv[0], argv);
	if(!rooted(argv[0])){
		rerrstr(err, sizeof err);
		exec(smprint("/bin/%s", argv[0]), argv);
		errstr(err, sizeof err);
	}
	sysfatal("exec: %s: %r", argv[0]);
}	
