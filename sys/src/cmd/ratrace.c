#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>

enum {
	Stacksize	= 8*1024,
	Bufsize		= 8*1024,
};

Channel *out;
Channel *quit;
Channel *forkc;
int nread = 0;

typedef struct Str Str;
struct Str {
	char	*buf;
	int	len;
};

void
die(char *s)
{
	fprint(2, "%s\n", s);
	exits(s);
}

void
cwrite(int fd, char *path, char *cmd, int len)
{
	if (write(fd, cmd, len) < len) {
		fprint(2, "cwrite: %s: failed %d bytes: %r\n", path, len);
		sendp(quit, nil);
		threadexits(nil);
	}
}

void
reader(void *v)
{
	int cfd, tfd, forking = 0, pid, newpid;
	char *ctl, *truss;
	Str *s;

	pid = (int)(uintptr)v;
	ctl = smprint("/proc/%d/ctl", pid);
	if ((cfd = open(ctl, OWRITE)) < 0)
		die(smprint("%s: %r", ctl));
	truss = smprint("/proc/%d/syscall", pid);
	if ((tfd = open(truss, OREAD)) < 0)
		die(smprint("%s: %r", truss));

	cwrite(cfd, ctl, "stop", 4);
	cwrite(cfd, truss, "startsyscall", 12);

	s = mallocz(sizeof(Str) + Bufsize, 1);
	s->buf = (char *)&s[1];
	while((s->len = pread(tfd, s->buf, Bufsize - 1, 0)) > 0){
		if (forking && s->buf[1] == '=' && s->buf[3] != '-') {
			forking = 0;
			newpid = strtol(&s->buf[3], 0, 0);
			sendp(forkc, (void*)newpid);
			procrfork(reader, (void*)newpid, Stacksize, 0);
		}

		/*
		 * There are three tests here and they (I hope) guarantee
		 * no false positives.
		 */
		if (strstr(s->buf, " Rfork") != nil) {
			char *a[8];
			char *rf;

			rf = strdup(s->buf);
         		if (tokenize(rf, a, 8) == 5) {
				ulong flags;

				flags = strtoul(a[4], 0, 16);
				if (flags & RFPROC)
					forking = 1;
			}
			free(rf);
		}
		sendp(out, s);
		cwrite(cfd, truss, "startsyscall", 12);
		s = mallocz(sizeof(Str) + Bufsize, 1);
		s->buf = (char *)&s[1];
	}
	sendp(quit, nil);
	threadexitsall(nil);
}

void
writer(void *)
{
	int newpid;
	Alt a[4];
	Str *s;

	a[0].op = CHANRCV;
	a[0].c = quit;
	a[0].v = nil;
	a[1].op = CHANRCV;
	a[1].c = out;
	a[1].v = &s;
	a[2].op = CHANRCV;
	a[2].c = forkc;
	a[2].v = &newpid;
	a[3].op = CHANEND;

	for(;;)
		switch(alt(a)){
		case 0:
			nread--;
			if(nread <= 0)
				goto done;
			break;
		case 1:
			/* it's a nice null terminated thing */
			fprint(2, "%s", s->buf);
			free(s);
			break;
		case 2:
			// procrfork(reader, (void*)newpid, Stacksize, 0);
			nread++;
			break;
		}
done:
	exits(nil);
}

void
usage(void)
{
	fprint(2, "Usage: ratrace [-c cmd [arg...]] | [pid]\n");
	exits("usage");
}

void
threadmain(int argc, char **argv)
{
	int pid;
	char *cmd = nil;
	char **args = nil;

	/*
	 * don't bother with fancy arg processing, because it picks up options
	 * for the command you are starting.  Just check for -c as argv[1]
	 * and then take it from there.
	 */
	if (argc < 2)
		usage();
	if (argv[1][0] == '-')
		switch(argv[1][1]) {
		case 'c':
			if (argc < 3)
				usage();
			cmd = strdup(argv[2]);
			args = &argv[2];
			break;
		default:
			usage();
		}

	/* run a command? */
	if(cmd) {
		pid = fork();
		if (pid < 0)
			sysfatal("fork failed: %r");
		if(pid == 0) {
			exec(cmd, args);
			if(cmd[0] != '/')
				exec(smprint("/bin/%s", cmd), args);
			sysfatal("exec %s failed: %r", cmd);
		}
	} else {
		if(argc != 2)
			usage();
		pid = atoi(argv[1]);
	}

	out   = chancreate(sizeof(char*), 0);
	quit  = chancreate(sizeof(char*), 0);
	forkc = chancreate(sizeof(ulong *), 0);
	nread++;
	procrfork(writer, nil, Stacksize, 0);
	reader((void*)pid);
}
