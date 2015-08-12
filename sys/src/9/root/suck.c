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

#define ESTR 256

static void
error(char *fmt, ...)
{
	va_list v;
	char *e, estr[ESTR], *p;

	va_start(v, fmt);
	e = estr + ESTR;
	p = seprint(estr, e, "%s: ", argv0);
	p = vseprint(p, e, fmt, v);
	p = seprint(p, e, "\n");
	va_end(v);

	write(2, estr, p - estr);
}

static void
fatal(char *fmt, ...)
{
	va_list v;
	char *e, estr[ESTR], *p;

	va_start(v, fmt);
	e = estr + ESTR;
	p = seprint(estr, e, "%s: ", argv0);
	p = vseprint(p, e, fmt, v);
	p = seprint(p, e, "\n");
	va_end(v);

	write(2, estr, p - estr);
	exits("fatal");
}

static void
usage(void)
{
	char *e, estr[ESTR], *p;

	e = estr + ESTR;
	p = seprint(estr, e, "usage: %s"
			     " [whatever]"
			     "\n",
		    argv0);
	write(2, estr, p - estr);
	exits("usage");
}

#define F(v, o, w) (((v) & ((1 << (w)) - 1)) << (o))

enum {
	X = 0, /* dimension */
	Y = 1,
	Z = 2,
	N = 3,

	Chunk = 32, /* granularity of FIFO */
	Pchunk = 8, /* Chunks in a packet */

	Quad = 16,
};

/*
 * Packet header. The hardware requires an 8-byte header
 * of which the last two are reserved (they contain a sequence
 * number and a header checksum inserted by the hardware).
 * The hardware also requires the packet to be aligned on a
 * 128-bit boundary for loading into the HUMMER.
 */
typedef struct Tpkt Tpkt;
struct Tpkt {
	uint8_t sk;     /* Skip Checksum Control */
	uint8_t hint;   /* Hint|Dp|Pid0 */
	uint8_t size;   /* Size|Pid1|Dm|Dy|VC */
	uint8_t dst[N]; /* Destination Coordinates */
	uint8_t _6_[2]; /* reserved */
	uint8_t _8_[8]; /* protocol header */
	uint8_t payload[];
};

/*
 * SKIP is a field in .sk giving the number of 2-bytes
 * to skip from the top of the packet before including
 * the packet bytes into the running checksum.
 * SIZE is a field in .size giving the size of the
 * packet in 32-byte 'chunks'.
 */
#define SKIP(n) F(n, 1, 7)
#define SIZE(n) F(n, 5, 3)

enum {
	Sk = 0x01, /* Skip Checksum */

	Pid0 = 0x01, /* Destination Group FIFO MSb */
	Dp = 0x02,   /* Multicast Deposit */
	Hzm = 0x04,  /* Z- Hint */
	Hzp = 0x08,  /* Z+ Hint */
	Hym = 0x10,  /* Y- Hint */
	Hyp = 0x20,  /* Y+ Hint */
	Hxm = 0x40,  /* X- Hint */
	Hxp = 0x80,  /* X+ Hint */

	Vcd0 = 0x00, /* Dynamic 0 VC */
	Vcd1 = 0x01, /* Dynamic 1 VC */
	Vcbn = 0x02, /* Deterministic Bubble VC */
	Vcbp = 0x03, /* Deterministic Priority VC */
	Dy = 0x04,   /* Dynamic Routing */
	Dm = 0x08,   /* DMA Mode */
	Pid1 = 0x10, /* Destination Group FIFO LSb */
};

static int
torusparse(uint8_t d[3], char *item, char *buf)
{
	int n;
	char *p;

	if((p = strstr(buf, item)) == nil || (p != buf && *(p - 1) != '\n'))
		return -1;
	n = strlen(item);
	if(strlen(p) < n + sizeof(": x 0 y 0 z 0"))
		return -1;
	p += n + sizeof(": x ") - 1;
	if(strncmp(p - 4, ": x ", 4) != 0)
		return -1;
	if((n = strtol(p, &p, 0)) > 255 || *p != ' ' || *(p + 1) != 'y')
		return -1;
	d[0] = n;
	if((n = strtol(p + 2, &p, 0)) > 255 || *p != ' ' || *(p + 1) != 'z')
		return -1;
	d[1] = n;
	if((n = strtol(p + 2, &p, 0)) > 255 || (*p != '\n' && *p != '\0'))
		return -1;
	d[2] = n;

	return 0;
}

static void
dumptpkt(Tpkt *tpkt, int hflag, int dflag)
{
	uint8_t *t;
	int i, j, n;
	char buf[512], *e, *p;

	n = ((tpkt->size >> 5) + 1) * Chunk;

	p = buf;
	e = buf + sizeof(buf);
	if(hflag) {
		p = seprint(p, e, "Hw:");
#ifdef notdef
		p = seprint(p, e, " sk %#2.2ux (Skip %d Sk %d)",
			    tpkt->sk, tpkt->sk & Sk, tpkt->sk >> 1);
		p = seprint(p, e, " hint %#2.2ux", tpkt->hint);
		p = seprint(p, e, " size %#2.2ux", tpkt->size);
		p = seprint(p, e, " dst [%d, %d, %d]",
			    tpkt->dst[X], tpkt->dst[Y], tpkt->dst[Z]);
		p = seprint(p, e, " _6_[0] %#2.2ux (seqno %d)",
			    tpkt->_6_[0], tpkt->_6_[0]);
		p = seprint(p, e, " _6_[1] %#2.2ux (crc)\n", tpkt->_6_[1]);
#else
		t = (uint8_t *)tpkt;
		for(i = 0; i < 8; i++)
			p = seprint(p, e, " %2.2ux", t[i]);
		p = seprint(p, e, "\n");
#endif /* notdef */

		p = seprint(p, e, "Sw:");
		t = (uint8_t *)tpkt->_8_;
		for(i = 0; i < 8; i++)
			p = seprint(p, e, " %#2.2ux", t[i]);
		print("%s\n", buf);
	}

	if(!dflag)
		return;

	n -= sizeof(Tpkt);
	for(i = 0; i < n; i += 16) {
		p = seprint(buf, e, "%4.4ux:", i);
		for(j = 0; j < 16; j++)
			seprint(p, e, " %2.2ux", tpkt->payload[i + j]);
		print("%s\n", buf);
	}
}

void
main(int argc, char *argv[])
{
	Tpkt *tpkt;
	u8int d[N];
	char buf[512], *p;
	uint64_t r, start, stop;
	int count, dflag, fd, i, hflag, length, mhz, n;

	count = 1;
	dflag = hflag = 0;
	length = Pchunk * Chunk;
	mhz = 700;

	ARGBEGIN
	{
	default:
		usage();
		break;
	case 'd':
		dflag = 1;
		break;
	case 'h':
		hflag = 1;
		break;
	case 'l':
		p = EARGF(usage());
		if((n = strtol(argv[0], &p, 0)) <= 0 || p == argv[0] || *p != 0)
			usage();
		if(n % Chunk)
			usage();
		length = n;
		if(length > Pchunk * Chunk) {
			n = (n + (Pchunk * Chunk) - 1) / (Pchunk * Chunk);
			length += (n - 1) * sizeof(Tpkt);
		}
		break;
	case 'm':
		p = EARGF(usage());
		if((n = strtol(argv[0], &p, 0)) <= 0 || p == argv[0] || *p != 0)
			usage();
		mhz = n;
		break;
	case 'n':
		p = EARGF(usage());
		if((n = strtol(argv[0], &p, 0)) <= 0 || p == argv[0] || *p != 0)
			usage();
		count = n;
		break;
	}
	ARGEND;

	if((fd = open("/dev/torusstatus", OREAD)) < 0)
		fatal("open /dev/torusstatus: %r\n");
	if((n = read(fd, buf, sizeof(buf))) < 0)
		fatal("read /dev/torusstatus: %r\n");
	close(fd);
	buf[n] = 0;

	if(torusparse(d, "addr", buf) < 0)
		fatal("parse /dev/torusstatus: <%s>\n", buf);
	print("addr: %d.%d.%d\n", d[X], d[Y], d[Z]);
	if(torusparse(d, "size", buf) < 0)
		fatal("parse /dev/torusstatus: <%s>\n", buf);
	print("size: %d.%d.%d\n", d[X], d[Y], d[Z]);

	if((tpkt = mallocalign(length, Chunk, 0, 0)) == nil)
		fatal("mallocalign tpkt\n");

	if((fd = open("/dev/torus", ORDWR)) < 0)
		fatal("open /dev/torus: %r\n");

	print("starting %d reads of %d\n", count, length);

	r = count * length;

	cycles(&start);
	for(i = 0; i < r; i += n) {
		if((n = pread(fd, tpkt, length, 0)) < 0)
			fatal("read /dev/torus: %r\n", n);
		if(hflag || dflag)
			dumptpkt(tpkt, hflag, dflag);
	}
	cycles(&stop);

	close(fd);

	r = (count * length);
	r *= mhz;
	r /= stop - start;

	print("%d reads in %llud cycles @ %dMHz = %llud MB/s\n",
	      i, stop - start, mhz, r);

	exits(0);
}
