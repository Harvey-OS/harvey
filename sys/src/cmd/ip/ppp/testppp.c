#include <u.h>
#include <libc.h>
#include <ctype.h>

int	debug;
long	errrate;
long	droprate;
int	framing;
int	nocompress;
int	noipcompress;
char	*ppp = "8.out";
char	*mtu;

void
pppopen(int fd, char *net, char *local, char *remote)
{
	char *argv[16];
	int argc;

	switch(fork()){
	case -1:
		fprint(2, "testppp: can't fork: %r\n");
		exits(0);
	case 0:
		return;
	default:
		break;
	}

	dup(fd, 0);
	dup(fd, 1);

	argc = 0;
	argv[argc++] = ppp;
	if(debug)
		argv[argc++] = "-d";
	if(framing)
		argv[argc++] = "-f";
	if(nocompress)
		argv[argc++] = "-c";
	if(noipcompress)
		argv[argc++] = "-C";
	if(mtu){
		argv[argc++] = "-m";
		argv[argc++] = mtu;
	}
	argv[argc++] = "-x";
	argv[argc++] = net;
	if(local){
		argv[argc++] = local;
		if(remote)
			argv[argc++] = remote;
	}
	argv[argc] = 0;
	exec(ppp, argv);
}

void
printbuf(uchar *p, int n)
{
	int i;
	uchar *e;
	char buf[32*5];

	if(n > 32)
		n = 32;
	
	i = 0;
	for(e = p + n; p < e; p++){
		if(isprint(*p))
			i += sprint(buf+i, "%c ", *p);
		else
			i += sprint(buf+i, "%2.2ux ", *p);
	}
	fprint(2, "%s\n", buf);
}

void
xfer(int from, int to)
{
	uchar buf[4096];
	int i, n, modified, ok, total, errs, dropped;

	if(fork() == 0)
		return;

	total = ok = errs = dropped = 0;
	for(;;){
		n = read(from, buf, sizeof(buf));
		if(n <= 0){
			fprint(2, "%d -> %d EOF\n", from, to);
			exits(0);
		}
		modified = 0;
		if(errrate){
			for(i = 0; i < n; i++){
				if(lnrand(errrate) == 0){
					buf[i] ^= 0xff;
					modified = 1;
				}
			}
		}
		if(droprate && lnrand(droprate) == 0){
			fprint(2, "!!!!!!!!!!!!!!%d -> %d dropped %d (%d/%d)\n", from, to, ok, dropped, total);
			ok = 0;
			dropped++;
			total++;
			continue;
		}
		if(modified){
			fprint(2, "!!!!!!!!!!!!!!%d -> %d %d (%d/%d)\n", from, to, ok, errs, total);
			ok = 0;
			errs++;
		} else
			ok++;
		total++;
		if(debug > 1){
			fprint(2, "%d -> %d (%d)", from, to, n);
			printbuf(buf, n);
		}
		n = write(to, buf, n);
		if(n < 0){
			fprint(2, "%d -> %d write err\n", from, to);
			exits(0);
		}
	}
}

void
usage(void)
{
	fprint(2, "usage: testppp [-cCDf] [-e errrate] [-d droprate] [-m mtu] [-p ppp]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *s;
	int pfd1[2];
	int pfd2[2];

	errrate = 0;
	droprate = 0;
	ARGBEGIN{
	case 'c':
		nocompress = 1;
		break;
	case 'C':
		noipcompress = 1;
		break;
	case 'd':
		s = ARGF();
		if(s)
			droprate = strtol(s, nil, 0);
		break;
	case 'D':
		debug++;
		break;
	case 'e':
		s = ARGF();
		if(s)
			errrate = strtol(s, nil, 0);
		break;
	case 'f':
		framing = 1;
		break;
	case 'm':
		mtu = ARGF();
		break;
	case 'p':
		ppp = ARGF();
		if(ppp == nil)
			usage();
		break;
	default:
		usage();
		break;
	}ARGEND
	if(argc)
		usage();

	pipe(pfd1);
	pipe(pfd2);

	bind("#I2", "/net.alt2", MCREATE);
	bind("#I1", "/net.alt", MCREATE);
	pppopen(pfd1[0], "/net.alt2", "135.104.99.1", "135.104.99.2");
	pppopen(pfd2[0], "/net.alt", 0, 0);

	close(pfd1[0]);
	close(pfd2[0]);

	xfer(pfd1[1], pfd2[1]);
	xfer(pfd2[1], pfd1[1]);
	exits(0);
}

