#include "awiki.h"

int debug;
int mapfd;
char *email;
char *dir;

void
usage(void)
{
	fprint(2, "usage: Wiki [-e email] [dir]\n");
	exits("usage");
}

void
threadmain(int argc, char **argv)
{
	char *s;
	Dir *d;

	rfork(RFNAMEG);
	ARGBEGIN{
	case 'D':
		debug++;
		break;
	case 'e':
		email = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc > 1)
		usage();
	if(argc == 1)
		dir = argv[0];
	else
		dir = "/mnt/wiki";

	if(chdir(dir) < 0){
		fprint(2, "chdir(%s) fails: %r\n", dir);
		threadexitsall(nil);
	}

	if((mapfd = open("map", ORDWR)) < 0){
		fprint(2, "open(map): %r\n");
		threadexitsall(nil);
	}

	if((d = dirstat("1")) == nil){
		fprint(2, "dirstat(%s/1) fails: %r\n", dir);
		threadexitsall(nil);
	}
	s = emalloc(strlen(d->name)+2);
	strcpy(s, d->name);
	strcat(s, "/");
	wikiopen(s, nil);
	threadexits(nil);
}
