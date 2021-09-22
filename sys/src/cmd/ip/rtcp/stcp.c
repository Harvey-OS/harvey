/*
 * Send tcp just for bandwidth testing, much like ttcp.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>

#define NAMELEN 32
#define NS(x)	((vlong)(x))
#define US(x)	(NS(x) * 1000LL)
#define MS(x)	(US(x) * 1000LL)
#define S(x)	(MS(x) * 1000LL)

int killed;		/* kill signal received */
int maxfragsize;
int persistent;
int quiet = 1;
int xfermode;		/* file transfer mode */

int
note_handler(void*, char*)
{
	killed = 1;
	return 1;
}

/*
 *  dial and return a data connection
 */
int
dodial(char *dest)
{
	int data, fd;
	char *name;
	char devdir[40], s[64];

	name = netmkaddr(dest, "tcp", "ttcp");
	do {
		data = dial(name, 0, devdir, 0);
		if(data < 0)
			sleep(500);
	} while(persistent && data < 0);
	if(data < 0)
		sysfatal("%r");

	if(!quiet)
		print("%s on %s: ", name, devdir);

	if(maxfragsize){
		snprint(s, sizeof s, "%s/ctl", devdir);
		if((fd = open(s, OWRITE)) < 0)
			sysfatal("%s: %r", s);
		if(fprint(fd, "maxfragsize %d\n", maxfragsize) < 0)
			sysfatal("%s: %r", s);
		close(fd);
	}
	return data;
}

void
usage(void)
{
	fprint(2, "Usage: %s [-qs] [-f file] [-F maxfrag] [-l writesize] "
		"[-T totsize] host\n", argv0);
	exits("usage");
}

void
eusage(char *msg)
{
	if(msg)
		fprint(2, "%s: %s\n", argv0, msg);
	usage();
}

void
main(int argc, char **argv)
{
	int fd = 0, cc, s, i, sflag = 0, writesize;
	long t0, t1;
	vlong total, totsize = 0;
	char *filename = nil, *hostname = nil, *p = nil;

	writesize = 8192;
	ARGBEGIN{
	case 'f':
		filename = EARGF(usage());
		break;
	case 'F':
		maxfragsize = strtol(EARGF(eusage(argv[0])), nil, 0);
		break;
	case 'l':
		writesize = atoi(EARGF(usage()));
		break;
	case 'q':
		quiet = persistent = 1;
		break;
	case 's':
		sflag = 1;
		break;
	case 'T':
		totsize = atoll(EARGF(usage()));
		break;
	case 'v':
		quiet = 0;
		break;
	default:
		usage();
	} ARGEND;

	if(argc > 0)
		hostname = argv[0];
	if(hostname == nil)
		eusage("no hostname");

again:
	s = dodial(hostname);
	if (p == nil) {
		p = malloc(writesize);
		if(p == nil)
	 		exits("stcp: malloc");

		for(i = 0; i < writesize; i++){
			p[i] = i;
			if(i & 1)
				p[i] = 0xaa;
		}
	}
	if(filename) {
		xfermode = 1;
		atnotify(note_handler, 1);

		if((fd = open(filename, OREAD)) < 0)
			sysfatal("can't open %s: %r", filename);
	}

	time(&t0);
	t1 = t0;
	total = 0;

	for(;;){
		vlong now;

		if(xfermode == 0){
			cc = writesize;
			now = nsec();
			memcpy(p, &now, sizeof(vlong));
		}else {
			if((cc = read(fd, p, writesize)) < 0) {
				if(killed)
					break;
				exits("read");
			}
			if(cc == 0)
				break;
		}
		if(write(s, p, cc) != cc){
			if(killed)
				break;
			if(xfermode) {		/* plan9 hangup */
				close(fd);
				goto again;
			}
			perror("stcp: send");
			break;
		}
		total += cc;
		if(totsize && total >= totsize)
			break;
	}

	if(killed) {
		strcpy(p, "stopdeadbeef");
		write(s, p, 13);
	}
	time(&t1);
	if(t1 == t0)
		t1 = t0 + 1;
	print("%,lld bytes / %ld s. = %g MB/s.\n",
	 	total, t1-t0, ((double)total/(1024*1024)) / (t1-t0));

	if(sflag) {		/* wait for the other side to close first */
		if(!killed) {
			strcpy(p, "donedeadbeef");
			write(s, p, 13);
		}
		while (read(s, p, 1) != 0) {	/* we might be killed */
			strcpy(p, "stopdeadbeef");
			write(s, p, 13);
		}
		time(&t1);
		print("receiver: %,lld bytes / %ld s. = %g MB/s.\n",
	 		total, t1-t0, ((double)total/(1024*1024)) / (t1-t0));
	}

	exits(0);
}
