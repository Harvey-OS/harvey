#include <u.h>
#include <libc.h>

static void
usage(void)
{
	fprint(2, "usage: %s [-s nkevs] [-f dev] [-o file] [-p pid] cmd [args...]\n", argv0);
	exits("usage");
}

char buf[1*1024*1024];
int done;
int pids[128];
int npids;

void
main(int argc, char *argv[])
{
	int fd, cfd, pid, ofd, i;
	char file[128], *f;
	char *dev, *devctl, *out;
	long tot, nr;
	int sz;

	sz = 8;
	dev = "/dev/ptrace";
	out = nil;
	ARGBEGIN{
	case 'f':
		dev = EARGF(usage());
		break;
	case 'o':
		out = EARGF(usage());
		break;
	case 's':
		sz = strtoul(EARGF(usage()), 0, 0);
		break;
	case 'p':
		if(npids > nelem(pids))
			sysfatal("too many pids");
		pids[npids++] = strtoul(EARGF(usage()), 0, 0);
		break;
	default:
		usage();
	}ARGEND
	if(argc == 0)
		usage();

	sz *= 1024;
	if(sz < 1024)
		sz = 1024;
	if(sz > 64*1024)	/* perhaps we shouldn't limit it */
		sz = 64*1024;

	devctl = smprint("%sctl", dev);
	if(devctl == nil)
		sysfatal("no memory");

	if(access(argv[0], AEXIST) < 0){
		seprint(file, file+sizeof file, "/bin/%s", argv[0]);
		f = file;
	}else
		f = argv[0];
	if(access(f, AEXIST) < 0)
		sysfatal("%s: %r", argv[0]);


	/* prepage; don't want interference while tracing */
	for(i = 0; i < sizeof buf; i += 4096)
		buf[i] = 0;

	rfork(RFREND);
	pid = 0;

	/* play a dance with the traced command so it starts when we want,
	 * or issue the trace command for the traced pids.
	 */
	cfd = open(devctl, OWRITE);
	if(cfd < 0)
		sysfatal("can't trace: %r");
	if(npids == 0){
		pid = rfork(RFPROC|RFFDG);
		switch(pid){
		case -1:
			sysfatal("fork: %r");
		case 0:
			/* wait until traced */
			if(rendezvous(usage, 0) == 0)
				exits("no trace");
			rfork(RFREND);
			exec(f, argv);
			sysfatal("exec: %r");
		}
		if(fprint(cfd, "size %d\n", sz) < 0 || fprint(cfd, "trace %d 1\n", pid) < 0){
			rendezvous(usage, 0);
			sysfatal("can't trace");
		}
	
		if(out == nil){
			rendezvous(usage, usage);
			waitpid();
			exits(nil);
		}
	}else{
		for(i = 0; i < npids; i++)
			if(fprint(cfd, "trace %d 1\n", pids[i]) < 0)
				fprint(2, "%s: trace %d: %r\n", argv0, pids[i]);
	}
	close(cfd);

	/* ready: open the trace and collect what we can. */
	fd = open(dev, OREAD);
	ofd = create(out, OWRITE, 0664);
	if(fd < 0 || ofd < 0){
		if(npids == 0)
			rendezvous(usage, 0);
		sysfatal("can't trace: %r");
	}

	done = 0;
	switch(rfork(RFPROC|RFMEM)){
	case -1:
		sysfatal("fork");
	default:
		if(npids == 0){
			/* let the traced go! */
			rendezvous(usage, usage);
		}

		while(waitpid() != pid)
			;
		done = 1;
		exits(nil);
	case 0:
		/* The damn device won't block us. We must insist upon eof.
		 * Besides, when done, we must give time for the events to
		 * arrive.
		 */
		for(;;){
			for(tot = 0; tot < sizeof buf - 128; tot += nr){
				nr = read(fd, buf+tot, sizeof buf - tot);
				if(nr < 0)
					sysfatal("read: %r");
				if(nr == 0){
					sleep(100);
					if(done)
						break;
				}
			}
			if(tot > 0 && write(ofd, buf, tot) != tot)
				sysfatal("write: %r");
			/* try to read more if we were done; but just once. */
			if(done && tot == 0 && done++ == 2)
				break;
		}
		close(fd);
		close(ofd);
		rendezvous(main, nil);
		exits(nil);
	}
}
