#include <u.h>
#include <libc.h>
#include <bio.h>

enum
{
	Doff=		4,	/* offset into Cpline.bytes of data */

	Memsize=	1<<16,	/* max size of 186 memory */
};

int dump, image, noload, nostart;

typedef struct
{
	int	type;
	int	dlen;
	ulong	addr;
	uchar	bytes[256+4];
	uchar	csum;
} Cpline;

char*	rdcpline(Biobuf*, Cpline*);
void	clearmem(int);

void
usage(void)
{
	fprint(2, "usage: %s [-0123] file\n", argv0);
	exits("usage");
}

static void
loadimage(char* file, int mfd)
{
	uchar buf[256];
	int fd, n, r;

	if((fd = open(file, OREAD)) < 0)
		sysfatal("opening %s: %r", file);

	seek(mfd, 0, 0);
	do{
		n = read(fd, buf, sizeof(buf));
		if(n < 0)
			sysfatal("read %s: %r", file);
		if(n > 0)
			if((r = write(mfd, buf, n)) != n)
				sysfatal("write %s: %d != %d: %r", file, n, r);
	}while(n > 0);
	close(fd);
}

static void
loadhex(char* file, int mfd)
{
	int done;
	Cpline c;
	Biobuf *b;
	char *err;
	ulong addr, seg;
	int lineno;
	uchar buf[1024];

	b = Bopen(file, OREAD);
	if(b == 0)
		sysfatal("opening %s: %r", file);

	lineno = 1;
	seg = 0;
	for(done = 0; !done; lineno++){
		err = rdcpline(b, &c);
		if(err)
			sysfatal("%s line %d: %s", file, lineno, err);
		switch(c.type){
		case 0: /* data */
			addr = seg + c.addr;
			if(addr + c.dlen > Memsize)
				sysfatal("addr out of range: %lux-%lux", addr, addr+c.dlen);
			if(seek(mfd, addr, 0) < 0)
				sysfatal("seeking to %lud: %r", addr);
			if(write(mfd, c.bytes+Doff, c.dlen) != c.dlen)
				sysfatal("writing: %r");
			if(seek(mfd, addr, 0) < 0)
				sysfatal("seeking to %lud: %r", addr);
			if(read(mfd, buf, c.dlen) != c.dlen)
				sysfatal("reading: %r");
			if(memcmp(buf, c.bytes+Doff, c.dlen) != 0)
				print("readback error at %lux\n", addr);
			if(dump)
				print("%8.8lux: %d\n", addr, c.dlen);
			break;
		case 1: /* termination */
			done = 1;
			break;
		case 2: /* segment */
			seg = ((c.bytes[Doff]<<8) | c.bytes[Doff+1]) <<4;
			if(seg >= Memsize)
				sysfatal("seg out of range: %lux", seg);
			if(dump)
				print("seg %8.8lux\n", seg);
			break;
		default: /* ignore */
			if(dump)
				print("bad type %d\n", c.type);
			break;
		}
	}
	Bterm(b);
}

void
main(int argc, char **argv)
{
	int unit;
	int cfd, mfd;
	char file[128];

	unit = 0;
	ARGBEGIN{
	case 'd':
		dump = 1;
		break;
	case 'i':
		image = 1;
		break;
	case 'n':
		noload = 1;
		break;
	case 's':
		nostart = 1;
		break;
	case '0':
		unit = 0;
		break;
	case '1':
		unit = 1;
		break;
	case '2':
		unit = 2;
		break;
	case '3':
		unit = 3;
		break;
	}ARGEND;

	if(argc == 0)
		usage();

	if(noload == 0){
		sprint(file, "#G/astar%dctl", unit);
		cfd = open(file, ORDWR);
		if(cfd < 0)
			sysfatal("opening %s", file);
		sprint(file, "#G/astar%dmem", unit);
		mfd = open(file, ORDWR);
		if(mfd < 0)
			sysfatal("opening %s", file);
	
		if(write(cfd, "download", 8) != 8)
			sysfatal("requesting download: %r");
	} else {
		cfd = -1;
		mfd = create("/tmp/astarmem", ORDWR, 0664);
		if(mfd < 0)
			sysfatal("creating /tmp/astarmem: %r");
	}

	if(image)
		loadimage(argv[0], mfd);
	else{
		/* zero out the memory */
		clearmem(mfd);
		loadhex(argv[0], mfd);
	}
	close(mfd);

	if(noload == 0 && nostart == 0)
		if(write(cfd, "run", 3) != 3)
			sysfatal("requesting run: %r");
	close(cfd);

	exits(0);
}

void
clearmem(int fd)
{
	char buf[4096];
	char buf2[4096];
	int i, n;

	memset(buf, 0, sizeof buf);
	for(i = 0; i < Memsize; i += n){
		if(seek(fd, i, 0) < 0)
			sysfatal("seeking to %ux: %r", i);
		n = write(fd, buf, sizeof buf);
		if(n <= 0)
			break;
		if(seek(fd, i, 0) < 0)
			sysfatal("seeking to %ux: %r", i);
		n = read(fd, buf2, sizeof buf2);
		if(n <= 0)
			break;
		if(memcmp(buf, buf2, sizeof buf) != 0)
			print("error zeroing mem at %ux\n", i);
	}
	print("zero'd %d bytes\n", i);
}

int
hex(char c)
{
	if(c <= '9' && c >= '0')
		return c - '0';
	if(c <= 'f' && c >= 'a')
		return (c - 'a') + 10;
	if(c <= 'F' && c >= 'A')
		return (c - 'A') + 10;
	return -1;
}

char*
rdcpline(Biobuf *b, Cpline *cpl)
{
	char *cp, *ep, *p;
	uchar *up;
	uchar csum;
	int c;

	cp = Brdline(b, '\n');
	if(cp == 0)
		return "early eof";
	ep = cp + Blinelen(b);

	if(*cp++ != ':')
		return "bad load line";

	csum = 0;
	up = cpl->bytes;
 	for(p = cp; p < ep;){
		c = hex(*p++)<<4;
		c |= hex(*p++);
		if(c < 0)
			break;
		csum += c;
		*up++ = c;
	}

	cpl->csum = csum;
	if(csum != 0){
		fprint(2, "checksum %ux\n", csum);
		return "bad checksum";
	}

	cpl->dlen = cpl->bytes[0];
	if(cpl->dlen + 5 != up - cpl->bytes){
		fprint(2, "%d %ld\n", cpl->dlen + 5, up - cpl->bytes);
		return "bad data length";
	}

	cpl->addr = (cpl->bytes[1]<<8) | cpl->bytes[2];
	cpl->type = cpl->bytes[3];
	return 0;
}
