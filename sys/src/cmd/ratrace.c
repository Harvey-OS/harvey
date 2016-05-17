/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
int debug = 0;
int outf = 2;

typedef struct Str Str;
struct Str {
	char	*buf;
	int	len;
};

void
die(char *s)
{
	fprint(outf, "%s\n", s);
	exits(s);
}

void
cwrite(int fd, char *path, char *cmd, int len)
{
	werrstr("");
	if (write(fd, cmd, len) < len) {
		fprint(outf, "cwrite: %s: failed writing %d bytes: %r\n",
			path, len);
		sendp(quit, nil);
		threadexits(nil);
	}
}

Str *
newstr(void)
{
	Str *s;

	s = mallocz(sizeof(Str) + Bufsize, 1);
	if (s == nil)
		sysfatal("malloc");
	s->buf = (char *)&s[1];
	return s;
}

void
reader(void *v)
{
	int cfd, tfd, exiting, pid;
	uintptr_t newpid;
	char *ctl, *truss;
	Str *s;
	static char waitstop[] = "waitstop";

	pid = (int)(uintptr)v;
	if (debug)
		fprint(outf, "DEBUG: -------------> reader starts with pid %d\n", pid);
	ctl = smprint("/proc/%d/ctl", pid);
	if ((cfd = open(ctl, OWRITE)) < 0)
		die(smprint("%s: %r", ctl));
	truss = smprint("/proc/%d/syscall", pid);
	if ((tfd = open(truss, OREAD)) < 0)
		die(smprint("%s: %r", truss));
	if (debug)
		fprint(outf, "DEBUG: Send %s to pid %d ...", waitstop, pid);
	/* child was stopped by hang msg earlier */
	cwrite(cfd, ctl, waitstop, sizeof waitstop - 1);
	if (debug)
		fprint(outf, "DEBUG: back for %d\n", pid);

	if (debug)
		fprint(outf, "DEBUG: Send %s to pid %d\n", "startsyscall", pid);
	cwrite(cfd, ctl, "startsyscall", 12);
	if (debug)
		fprint(outf, "DEBUG: back for %d\n", pid);
	s = newstr();
	exiting = 0;
	while((s->len = pread(tfd, s->buf, Bufsize - 1, 0)) >= 0){
		if (s->buf[0] == 'F') {
			char *val = strstr(s->buf, "= ");
			if (val) {
				newpid = strtol(&val[2], 0, 0);
				sendp(forkc, (void*)newpid);
				procrfork(reader, (void*)newpid, Stacksize, 0);
			}
		}

		if (strstr(s->buf, " Exits") != nil)
			exiting = 1;

		sendp(out, s);	/* print line from /proc/$child/syscall */
		if (exiting) {
			s = newstr();
			strcpy(s->buf, "\n");
			sendp(out, s);
			break;
		}

		/* flush syscall trace buffer */
		if (debug)
			fprint(outf, "DEBUG: Send %s to pid %d\n", "startsyscall", pid);
		cwrite(cfd, ctl, "startsyscall", 12);
		if (debug)
			fprint(outf, "DEBUG: back for %d\n", pid);
		s = newstr();
	}

	sendp(quit, nil);
	threadexitsall(nil);
}

void
writer(void *v)
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
		case 0:			/* quit */
			nread--;
			if(nread <= 0)
				goto done;
			break;
		case 1:			/* out */
			fprint(outf, "%s", s->buf);
			free(s);
			break;
		case 2:			/* forkc */
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
	fprint(2, "Usage: ratrace [-o file] [-d] [-c cmd [arg...]] | [pid]\n");
	exits("usage");
}

void
hang(void)
{
	int me;
	char *myctl;
	static char hang[] = "hang";

	myctl = smprint("/proc/%d/ctl", getpid());
	me = open(myctl, OWRITE);
	if (me < 0)
		sysfatal("can't open %s: %r", myctl);
	cwrite(me, myctl, hang, sizeof hang - 1);
	close(me);
	free(myctl);
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
	++argv;
	--argc;
	while (argv[0][0] == '-') {
		switch(argv[0][1]) {
		case 'c':
			if (argc < 2)
				usage();
			cmd = strdup(argv[1]);
			args = &argv[1];
			break;
		case 'd':
			debug = 1;
			break;
		case 'o':
			if (argc < 2)
				usage();
			outf = create(argv[1], OWRITE, 0666);
			if (outf < 0)
				sysfatal(smprint("%s: %r", argv[1]));
			++argv;
			--argc;
			break;
		default:
			usage();
		}
		++argv;
		--argc;
	}

	/* run a command? */
	if(cmd) {
		pid = fork();
		if (pid < 0)
			sysfatal("fork failed: %r");
		if(pid == 0) {
			hang();
			exec(cmd, args);
			if(cmd[0] != '/')
				exec(smprint("/bin/%s", cmd), args);
			sysfatal("exec %s failed: %r", cmd);
		}
	} else {
		if(argc != 1)
			usage();
		pid = atoi(argv[0]);
	}

	out   = chancreate(sizeof(char*), 0);
	quit  = chancreate(sizeof(char*), 0);
	forkc = chancreate(sizeof(uint32_t *), 0);
	nread++;
	procrfork(writer, nil, Stacksize, 0);
	reader((void*)(uintptr_t)pid);
}
