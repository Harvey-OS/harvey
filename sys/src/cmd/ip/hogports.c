#include <u.h>
#include <libc.h>

void
hogport(char *proto, int port)
{
	char buf[256];
	char dir[40];

	snprint(buf, sizeof(buf), "%s!%d", proto, port);
	if(announce(buf, dir) < 0)
		fprint(2, "%s: can't hog %s\n", argv0, buf);
}

void
hogrange(char *str)
{
	char *er, *sr;
	int start, end;

	sr = strrchr(str, '!');
	if(sr == nil)
		sysfatal("bad range: %s", str);
	*sr++ = 0;

	er = strchr(sr, '-');
	if(er == nil)
		er = sr;
	else
		er++;

	start = atoi(sr);
	end = atoi(er);
	if(end < start)
		sysfatal("bad range: %s", sr);

	for(; start <= end; start++)
		hogport(str, start);
}

void
main(int argc, char **argv)
{
	int i;

	ARGBEGIN{
	}ARGEND;

	if(argc == 0){
		fprint(2, "usage: %s portrange\n", argv0);
		exits("usage");
	}

	switch(rfork(RFREND|RFNOTEG|RFFDG|RFPROC|RFNAMEG)){
	case 0:
		close(0);
		close(1);
		break;
	case -1:
		abort(); /* "fork failed\n" */;
	default:
		_exits(0);
	}

	for(i = 0; i < argc; i++)
		hogrange(argv[i]);

	close(2);
	for(;;)
		sleep(10000);
}
