#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "9p1.h"

#define	MAXFDATA	(8*1024)
#define	MAXRPC		(MAXFDATA+160)

/*
 * reassemble 9P messages for stream based protocols
 * interposed between devmnt and the network by srv for tcp connections
 * fcall expects devmnt on fd0, network fd1
 */
uchar msglen[256] =
{
	[Tnop9p1]		3,
	[Rnop9p1]		3,
	[Tsession9p1]	3+CHALLEN,
	[Rsession9p1]	3+NAMEREC+DOMLEN+CHALLEN,
	[Terror9p1]	0,
	[Rerror9p1]	67,
	[Tflush9p1]	5,
	[Rflush9p1]	3,
	[Tattach9p1]	5+2*NAMEREC+TICKETLEN+AUTHENTLEN,
	[Rattach9p1]	13+AUTHENTLEN,
	[Tclone9p1]	7,
	[Rclone9p1]	5,
	[Twalk9p1]		33,
	[Rwalk9p1]		13,
	[Topen9p1]		6,
	[Ropen9p1]		13,
	[Tcreate9p1]	38,
	[Rcreate9p1]	13,
	[Tread9p1]		15,
	[Rread9p1]		8,
	[Twrite9p1]	16,
	[Rwrite9p1]	7,
	[Tclunk9p1]	5,
	[Rclunk9p1]	5,
	[Tremove9p1]	5,
	[Rremove9p1]	5,
	[Tstat9p1]		5,
	[Rstat9p1]		121,
	[Twstat9p1]	121,
	[Rwstat9p1]	5,
	[Tclwalk9p1]	35,
	[Rclwalk9p1]	13,
};

enum
{
	Twritehdr	= 16,	/* Min bytes for Twrite */
	Rreadhdr	= 8,	/* Min bytes for Rread */
	Twritecnt	= 13,	/* Offset in byte stream of write count */
	Rreadcnt	= 5,	/* Offset for Readcnt */
};

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
	case Twrite9p1:			/* Fmt: TGGFFOOOOOOOOCC */
		len = Twritehdr;	/* T = type, G = tag, F = fid */
		off = Twritecnt;	/* O = offset, C = count */
		break;
	case Rread9p1:			/* Fmt: TGGFFCC */
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
fcall(int fd)
{
	int i, r, n, l;
	uchar *p, *buf;
	int pipefd[2];

	if(pipe(pipefd) < 0)
		fatal("fcall pipe: %r");

	buf = malloc(MAXRPC);
	if(buf == nil)
		fatal("fcall malloc");

	switch(rfork(RFPROC|RFMEM|RFFDG|RFCNAMEG)){
	default:
		return pipefd[0];	/* parent returns fd */	
	case 0:
		break;	/* child builds buffers */
	case -1:
		fatal("fcall fork: %r");
	}

	/* close file descriptors */
	for(i=0; i<20; i++)
		if(i!=fd && i!=pipefd[1])
			close(i);

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

			if(write(pipefd[1], buf, r) < 0)
				break;

			n = (p - buf) - r;
			memmove(buf, buf+r, n);
			p = buf+n;
			l = MAXRPC - n;
		}
	}
	close(pipefd[1]);
	fatal(nil);
	return -1;
}
