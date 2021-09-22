/*
 * Simple write throughput benchmark
 */

#include <u.h>
#include <libc.h>

int
connect(char *name)
{
	char devdir[40];

	if(strchr(name, '!') == nil){
		return open(name, OWRITE);
	}
	return dial(name, 0, devdir, 0);
}

void
usage(void)
{
	fprint(2, "Usage: %s  [-l writesize] [-T totsize] file|ether|/net/{tcp|udp}\n", argv0);
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
	int fd, writesize;
	vlong t0, t1;
	vlong total, totsize;
	char *target, *p;

	totsize = 1024*1024*1024;
	target = nil;
	writesize = 8192;
	USED(target);
	target = "/dev/null";
	ARGBEGIN{
	case 'l':
		writesize = atoi(EARGF(usage()));
		break;
	case 'T':
		totsize = atoll(EARGF(usage()));
		break;
	default:
		usage();
	} ARGEND;
	if(argc > 0)
		target = argv[0];
	if(target == nil)
		eusage("no target");
	fd = connect(target);
	if(fd < 0)
		sysfatal("couldn't open: %s: %r", target);
	p = malloc(writesize);
	if(p == nil)
	 	sysfatal("wrbench: malloc: %r");
	t0 = nsec();
	total = 0;
	for(;;){
		if(write(fd, p, writesize) != writesize){
			perror("wrbench: write");
			break;
		}
		total += writesize;
		if(totsize && total >= totsize)
			break;
	}

	t1 = nsec();
	if(t1 == t0)
		t1 = t0+1;
	print("%lld %lld\n", t1-t0, total);

	exits(0);
}
