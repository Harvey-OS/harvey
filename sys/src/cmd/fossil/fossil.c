#include "stdinc.h"

#include "9.h"

int Dflag;
char* none = "none";
int stdfd[2];

static char* myname = "numpty";

static void
usage(void)
{
	argv0 = myname;
	sysfatal("usage: %s"
		" [-Dt]"
		" [-c cmd]",
		myname);
}

void
main(int argc, char* argv[])
{
	char **cmd, *p;
	int i, ncmd, tflag;

	myname = argv[0];
	fmtinstall('D', dirfmt);
	fmtinstall('F', fcallfmt);
	fmtinstall('M', dirmodefmt);
	quotefmtinstall();

	/*
	 * Insulate from the invoker's environment.
	 */
	if(rfork(RFREND|RFNOTEG|RFNAMEG) < 0)
		sysfatal("rfork: %r");

	close(0);
	open("/dev/null", OREAD);
	close(1);
	open("/dev/null", OWRITE);

	cmd = nil;
	ncmd = tflag = 0;

	vtAttach();

	ARGBEGIN{
	case '?':
	default:
		usage();
		break;
	case 'D':
		Dflag ^= 1;
		break;
	case 'c':
		p = ARGF();
		if(p == nil)
			usage();
		cmd = vtMemRealloc(cmd, (ncmd+1)*sizeof(char*));
		cmd[ncmd++] = p;
		break;
	case 't':
		tflag = 1;
		break;
	}ARGEND
	if(argc != 0)
		usage();

	consInit();
	cliInit();
	msgInit();
	conInit();
	cmdInit();
	fsysInit();
	exclInit();
	fidInit();

	srvInit();
	lstnInit();
	usersInit();

	for(i = 0; i < ncmd; i++){
		if(cliExec(cmd[i]) == 0)
			break;
	}
	vtMemFree(cmd);

	if(tflag && consTTY() == 0)
		consPrint("%s\n", vtGetError());

	vtDetach();
	exits(0);
}
