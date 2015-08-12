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

void
main(int argc, char *argv[])
{
	Tpkt *tpkt;
	u8int d[N];
	char buf[512], *p;
	uint64_t r, start, stop;
	int count, fd, i, length, mhz, n, x, y, z;

	count = 1;
	length = Pchunk * Chunk;
	mhz = 700;

	ARGBEGIN
	{
	default:
		usage();
		break;
	case 'l':
		p = EARGF(usage());
		if((n = strtol(argv[0], &p, 0)) <= 0 || p == argv[0] || *p != 0)
			usage();
		if(n % Chunk)
			usage();
		length = n;
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

	if(argc != 3)
		usage();
	if((x = strtol(argv[0], &p, 0)) < 0 || *p != 0)
		fatal("x invalid: %d\n", argv[0]);
	if((y = strtol(argv[1], &p, 0)) < 0 || *p != 0)
		fatal("y invalid: %d\n", argv[1]);
	if((z = strtol(argv[2], &p, 0)) <= 0 || *p != 0)
		fatal("z invalid: %d\n", argv[2]);
	z -= 1;

	if((fd = open("/dev/torusstatus", OREAD)) < 0)
		fatal("open /dev/torusstatus: %r\n");
	if((n = read(fd, buf, sizeof(buf))) < 0)
		fatal("read /dev/torusstatus: %r\n");
	close(fd);
	buf[n] = 0;

	if(torusparse(d, "size", buf) < 0)
		fatal("parse /dev/torusstatus: <%s>\n", buf);
	if(x >= d[X] || y >= d[Y] || z >= d[Z])
		fatal("destination out of range: %d.%d.%d >= %d.%d.%d",
		      x, y, z, d[X], d[Y], d[Z]);

	if((tpkt = mallocalign(length, Chunk, 0, 0)) == nil)
		fatal("mallocalign tpkt\n");
	memset(tpkt, 0, length);

	tpkt->sk = SKIP(4);
	tpkt->hint = 0;
	tpkt->size = SIZE(Pchunk - 1) | Dy | Vcd0;
	tpkt->dst[X] = x;
	tpkt->dst[Y] = y;
	tpkt->dst[Z] = z;

	if((fd = open("/dev/torus", ORDWR)) < 0)
		fatal("open /dev/torus: %r\n");

	cycles(&start);
	for(i = 0; i < count; i++) {
		n = pwrite(fd, tpkt, length, 0);
		if(n < 0)
			fatal("write /dev/torus: %r\n", n);
		else if(n < length)
			fatal("write /dev/torus: short write %d\n", n);
	}
	cycles(&stop);

	close(fd);

	r = (count * length);
	r *= mhz;
	r /= stop - start;

	print("%d writes of %d in %llud cycles @ %dMHz = %llud MB/s\n",
	      count, length, stop - start, mhz, r);

	exits(0);
}
