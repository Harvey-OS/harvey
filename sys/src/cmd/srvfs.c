#include <u.h>
#include <libc.h>

static void
usage(void)
{
	fprint(2, "usage: %s [-dR] [-p perm] [-P patternfile] [-e exportfs] srvname path\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *ename, *arglist[16], **argp;
	int n, fd, pipefd[2];
	char buf[64];
	int perm = 0600;

	argp = arglist;
	ename = "/bin/exportfs";
	*argp++ = "exportfs";
	ARGBEGIN{
	default:
		usage();
	case 'd':
		*argp++ = "-d";
		break;
	case 'e':
		ename = ARGF();
		break;
	case 'p':
		perm = strtol(ARGF(), 0, 8);
		break;
	case 'P':
		*argp++ = "-P";
		*argp++ = ARGF();
		break;
	case 'R':
		*argp++ = "-R";
		 break;
	}ARGEND
	*argp = 0;
	if(argc != 2)
		usage();

	if(pipe(pipefd) < 0){
		fprint(2, "can't pipe: %r\n");
		exits("pipe");
	}

	switch(rfork(RFPROC|RFNOWAIT|RFNOTEG|RFFDG)){
	case -1:
		fprint(2, "can't rfork: %r\n");
		exits("rfork");
	case 0:
		dup(pipefd[0], 0);
		dup(pipefd[0], 1);
		close(pipefd[0]);
		close(pipefd[1]);
		exec(ename, arglist);
		fprint(2, "can't exec exportfs: %r\n");
		exits("exec");
	default:
		break;
	}
	close(pipefd[0]);
	if(fprint(pipefd[1], "%s", argv[1]) < 0){
		fprint(2, "can't write pipe: %r\n");
		exits("write");
	}
	n = read(pipefd[1], buf, sizeof buf-1);
	if(n < 0){
		fprint(2, "can't read pipe: %r\n");
		exits("read");
	}
	buf[n] = 0;
	if(n != 2 || strcmp(buf, "OK") != 0){
		fprint(2, "not OK (%d): %s\n", n, buf);
		exits("OK");
	}
	if(argv[0][0] == '/')
		strecpy(buf, buf+sizeof buf, argv[0]);
	else
		snprint(buf, sizeof buf, "/srv/%s", argv[0]);
	fd = create(buf, OWRITE, perm);
	if(fd < 0){
		fprint(2, "can't create %s: %r\n", buf);
		exits("create");
	}
	fprint(fd, "%d", pipefd[1]);
	close(fd);
	close(pipefd[1]);
	exits(0);
}
