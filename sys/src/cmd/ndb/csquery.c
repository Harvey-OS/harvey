#include <u.h>
#include <libc.h>
#include <bio.h>

char *server;
char *status;
int statusonly;

void
usage(void)
{
	fprint(2, "usage: ndb/csquery [/net/cs [addr...]]\n");
	exits("usage");
}

void
query(char *addr)
{
	char buf[128];
	int fd, n;

	fd = open(server, ORDWR);
	if(fd < 0)
		sysfatal("cannot open %s: %r", server);
	if(write(fd, addr, strlen(addr)) <= 0){
		if(!statusonly)
			fprint(2, "translating %s: %r\n", addr);
		status = "errors";
		close(fd);
		return;
	}
	if(!statusonly){
		seek(fd, 0, 0);
		while((n = read(fd, buf, sizeof(buf)-1)) > 0){
			buf[n] = 0;
			print("%s\n", buf);
		}
	}
	close(fd);
}

void
main(int argc, char **argv)
{
	char *p;
	int i;
	Biobuf in;

	ARGBEGIN{
	case 's':
		statusonly = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc > 0)
		server = argv[0];
	else
		server = "/net/cs";

	if(argc > 1){
		for(i=1; i<argc; i++)
			query(argv[i]);
		exits(status);
	}

	Binit(&in, 0, OREAD);
	for(;;){
		print("> ");
		p = Brdline(&in, '\n');
		if(p == 0)
			break;
		p[Blinelen(&in)-1] = 0;
		query(p);
	}
	exits(nil);
}
