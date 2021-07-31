#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <ip.h>

#include "boot.h"

/*
 * reassemble 9P messages for stream based protocols
 * interposed between devmnt and the network by srv for tcp connections
 * fcall expects devmnt on fd0, network fd1
 */
static uchar msglen[256] =
{
	[Tnop]		3,
	[Rnop]		3,
	[Tsession]	3+CHALLEN,
	[Rsession]	3+NAMELEN+DOMLEN+CHALLEN,
	[Terror]	0,
	[Rerror]	67,
	[Tflush]	5,
	[Rflush]	3,
	[Tattach]	5+2*NAMELEN+TICKETLEN+AUTHENTLEN,
	[Rattach]	13+AUTHENTLEN,
	[Tclone]	7,
	[Rclone]	5,
	[Twalk]		33,
	[Rwalk]		13,
	[Topen]		6,
	[Ropen]		13,
	[Tcreate]	38,
	[Rcreate]	13,
	[Tread]		15,
	[Rread]		8,
	[Twrite]	16,
	[Rwrite]	7,
	[Tclunk]	5,
	[Rclunk]	5,
	[Tremove]	5,
	[Rremove]	5,
	[Tstat]		5,
	[Rstat]		121,
	[Twstat]	121,
	[Rwstat]	5,
	[Tclwalk]	35,
	[Rclwalk]	13,
};

enum
{
	Twritehdr	= 16,	/* Min bytes for Twrite */
	Rreadhdr	= 8,	/* Min bytes for Rread */
	Twritecnt	= 13,	/* Offset in byte stream of write count */
	Rreadcnt	= 5,	/* Offset for Readcnt */
};

void
echo(int f, int t)
{
	int n;
	char buf[MAXRPC];

	for(;;) {
		n = read9p(f, buf, MAXRPC);
		if(n <= 0)
			break;
		if(write(t, buf, n) < n)
			break;
	}
}

int
mntrpclen(uchar *d, int n)
{
	uchar t;
	int len, off;

	if(n < 1)
		return 0;

	t = d[0];
	switch(t) {			/* This is the type */
	default:
		len = msglen[t];
		if(len == 0)		/* Illegal type so consume */
			return n;
		if(n < len)
			return 0;
		return len;
	case Twrite:			/* Fmt: TGGFFOOOOOOOOCC */
		len = Twritehdr;	/* T = type, G = tag, F = fid */
		off = Twritecnt;	/* O = offset, C = count */
		break;
	case Rread:			/* Fmt: TGGFFCC */
		len = Rreadhdr;
		off = Rreadcnt;
		break;
	}
	if(n < off+2)
		return 0;

	len += d[off]|(d[off+1]<<8);
	if(n < len)
		return 0;

	return len;
}

int
pushfcall(int fd)
{
	int r, n, l, pfd[2];
	uchar *p, buf[MAXRPC];

	if(pipe(pfd) < 0){
		fprint(2, "pipe: %r\n");
		exits("pipe");
	}

	/* Downstream is just a copy process */
	switch(rfork(RFPROC|RFFDG|RFMEM)){
	case 0:
		close(pfd[1]);
		echo(pfd[0], fd);
		fprint(2, "echo finished\n");
		exits("echo done");
	case -1:
		fprint(2, "fcall fork failed: %r\n");
		exits("fork");
	}

	switch(rfork(RFPROC|RFFDG|RFMEM)){
	default:
		close(pfd[0]);
		return pfd[1];
	case 0:
		close(pfd[1]);
		break;
	case -1:
		fprint(2, "fcall fork failed: %r\n");
		exits("fork");
	}

	/* Dispatch only full RPC's to the mount driver */
	l = MAXRPC;
	p = buf;
	for(;;) {
		n = read(fd, p, l);
		if(n < 0)
			break;
		p += n;
		l -= n;

		for(;;) {
			r = mntrpclen(buf, p - buf);
			if(r == 0)
				break;

			if(write(pfd[0], buf, r) < 0)
				break;

			n = (p - buf) - r;
			memmove(buf, buf+r, n);
			p = buf+n;
			l = MAXRPC - n;
		}
	}
	abort();
	return 0;
}
