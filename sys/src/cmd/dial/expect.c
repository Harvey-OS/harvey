#include <u.h>
#include <libc.h>

void
usage(void)
{
	fprint(2, "usage: %s [-q] [-t secs] goodstring [badstring ...]\n", argv0);
	exits("usage");
}

void
catch(void*, char *s)
{
	exits(s);
}

int
writewithoutcr(int fd, char *p, int i)
{
	char *q, *e;

	/* dump cr's */
	for(e = p+i; p < e; ){
		q = memchr(p, '\r', e-p);
		if(q == nil)
			break;
		if(q > p)
			if(write(fd, p, q-p) < 0)
				return -1;
		p = q+1;
	}
	if(p < e)
		if(write(fd, p, e-p) < 0)
			return -1;
	return i;
}

void
main(int argc, char **argv)
{
	int timeout = 5*60;
	int quiet = 0;
	int ignorecase = 0;
	int fd, i, m, n, bsize;
	char *good;
	char *buf;
	int sofar;

	ARGBEGIN {
	case 'i':
		ignorecase = 1;
		break;
	case 't':
		timeout = atoi(EARGF(usage()));
		break;
	case 'q':
		quiet = 1;
		break;
	} ARGEND;

	if(argc < 1)
		usage();

	good = argv[0];
	n = strlen(good);

	for(i = 1; i < argc; i++){
		m = strlen(argv[i]);
		if(m > n)
			n = m;
	}

	fd = open("/dev/cons", ORDWR);
	if(fd < 0)
		sysfatal("opening /dev/cons: %r");

	bsize = n+4096;
	buf = malloc(bsize+1);

	sofar = 0;
	alarm(timeout*1000);
	for(;;){
		if(sofar > n){
			memmove(buf, &buf[sofar-n], n);
			sofar = n;
		}
		i = read(0, buf+sofar, bsize);
		if(i <= 0)
			exits("EOF");
		if(!quiet)
			writewithoutcr(fd, buf+sofar, i);
		sofar += i;
		buf[sofar] = 0;
		if(ignorecase){
			if(cistrstr(buf, good))
				break;
			for(i = 1; i < argc; i++)
				if(cistrstr(buf, argv[i]))
					exits(argv[i]);
		} else {
			if(strstr(buf, good))
				break;
			for(i = 1; i < argc; i++)
				if(strstr(buf, argv[i]))
					exits(argv[i]);
		}
	}

	exits(0);
}
