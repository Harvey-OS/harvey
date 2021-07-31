#include <u.h>
#include <libc.h>
#include <a.out.h>
#include <bio.h>

void	record(uchar*, int);
void	usage(void);
void	segment(long, int);

enum
{
	Recordsize = 32,
};

int	dsegonly;
int	supressend;
int	binary;
ulong	addr;
ulong 	psize = 4096;
Biobuf 	stdout;
Exec	exech;
Biobuf *bio;

void
main(int argc, char **argv)
{
	int n;
	Dir dir;

	ARGBEGIN{
	case 'd':
		dsegonly++;
		break;
	case 's':
		supressend++;
		break;
	case 'a':
		addr = strtoul(ARGF(), 0, 0);
		break;
	case 'p':
		psize = strtoul(ARGF(), 0, 0);
		break;
	case 'b':
		binary++;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();

	Binit(&stdout, 1, OWRITE);

	bio = Bopen(argv[0], OREAD);
	if(bio == 0) {
		fprint(2, "ms2: open %s: %r\n", argv[0]);
		exits("open");
	}

	if(binary) {
		if(dirfstat(Bfildes(bio), &dir) < 0) {
			fprint(2, "ms2: stat failed %r");
			exits("dirfstat");
		}
		segment(0, dir.length);
		Bprint(&stdout, "S9030000FC\n");
		Bterm(&stdout);
		Bterm(bio);
		exits(0);
	}

	n = Bread(bio, &exech, sizeof(Exec));
	if(n != sizeof(Exec)) {
		fprint(2, "ms2: read failed: %r\n");
		exits("read");
	}

	switch(exech.magic) {
	default:
		fprint(2, "ms2: bad magic\n");
		exits("magic");
	case A_MAGIC:
	case I_MAGIC:
	case J_MAGIC:
	case K_MAGIC:
	case V_MAGIC:
		break;
	}

	if(dsegonly)
		segment(exech.text+sizeof(exech), exech.data);
	else {
		segment(sizeof(exech), exech.text);
		addr = (addr+(psize-1))&~(psize-1);
		segment(exech.text+sizeof(exech), exech.data);
	}

	if(supressend == 0)
		Bprint(&stdout, "S9030000FC\n");

	Bterm(&stdout);
	Bterm(bio);
	exits(0);
}

void
segment(long foff, int len)
{
	int l, n;
	uchar buf[2*Recordsize];

	Bseek(bio, foff, 0);
	for(;;) {
		l = len;
		if(l > Recordsize)
			l = Recordsize;
		n = Bread(bio, buf, l);
		if(n == 0)
			break;
		if(n < 0) {
			fprint(2, "ms2: read error: %r\n");
			exits("read");
		}
		record(buf, l);
		len -= l;
	}
}

void
record(uchar *s, int l)
{
	int i;
	ulong cksum;

	cksum = l+4;
	Bprint(&stdout, "S2%.2X%.6X", l+4, addr);
	cksum += addr&0xff;
	cksum += (addr>>8)&0xff;
	cksum += (addr>>16)&0xff;

	for(i = 0; i < l; i++) {
		cksum += *s;
		Bprint(&stdout, "%.2X", *s++);
	}
	Bprint(&stdout, "%.2X\n", (~cksum)&0xff);
	addr += l;
}

void
usage(void)
{
	fprint(2, "usage: ms2 [-ds] [-a address] [-p pagesize] ?.out\n");
	exits("usage");
}
