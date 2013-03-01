#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

void	record(uchar*, long);
void	usage(void);
void	segment(vlong, vlong);

enum
{
	Recordsize = 32,
};

int	dsegonly;
int	supressend;
int	binary;
int	halfswap;
int	srec = 2;
uvlong	addr;
uvlong 	psize = 4096;
Biobuf 	stdout;
Fhdr	exech;
Biobuf *bio;

void
main(int argc, char **argv)
{
	Dir *dir;
	uvlong totsz;

	ARGBEGIN{
	case 'd':
		dsegonly++;
		break;
	case 's':
		supressend++;
		break;
	case 'a':
		addr = strtoull(ARGF(), 0, 0);
		break;
	case 'p':
		psize = strtoul(ARGF(), 0, 0);
		break;
	case 'b':
		binary++;
		break;
	case 'h':
		halfswap++;
		break;
	case '1':
		srec = 1;
		break;
	case '2':
		srec = 2;
		break;
	case '3':
		srec = 3;
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
		if((dir = dirfstat(Bfildes(bio))) == nil) {
			fprint(2, "ms2: stat failed %r");
			exits("dirfstat");
		}
		segment(0, dir->length);
		Bprint(&stdout, "S9030000FC\n");
		Bterm(&stdout);
		Bterm(bio);
		free(dir);
		exits(0);
	}

	if(!crackhdr(Bfildes(bio), &exech)) {
		fprint(2, "ms2: can't decode file header\n");
		return;
	}

	totsz = exech.txtsz + exech.datsz + exech.bsssz;
	fprint(2, "%s: %lud+%lud+%lud=%llud\n",
		exech.name, exech.txtsz, exech.datsz, exech.bsssz, totsz);

	if(dsegonly)
		segment(exech.datoff, exech.datsz);
	else {
		segment(exech.txtoff, exech.txtsz);
		addr = (addr+(psize-1))&~(psize-1);
		segment(exech.datoff, exech.datsz);
	}

	if(supressend == 0) {
		switch(srec) {
		case 1:
		case 2:
			Bprint(&stdout, "S9030000FC\n");
			break;
		case 3:
			Bprint(&stdout, "S705000000FA\n");
			break;
		}
	}

	Bterm(&stdout);
	Bterm(bio);
	exits(0);
}

void
segment(vlong foff, vlong len)
{
	int i;
	long l, n;
	uchar t, buf[2*Recordsize];

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
		if(halfswap) {
			if(n & 1) {
				fprint(2, "ms2: data must be even length\n");
				exits("even");
			}
			for(i = 0; i < n; i += 2) {
				t = buf[i];
				buf[i] = buf[i+1];
				buf[i+1] = t;
			}
		}
		record(buf, l);
		len -= l;
	}
}

void
record(uchar *s, long l)
{
	int i;
	ulong cksum = 0;

	switch(srec) {
	case 1:
		cksum = l+3;
		Bprint(&stdout, "S1%.2lX%.4lluX", l+3, addr);
		cksum += addr&0xff;
		cksum += (addr>>8)&0xff;
		break;
	case 2:
		cksum = l+4;
		Bprint(&stdout, "S2%.2lX%.6lluX", l+4, addr);
		cksum += addr&0xff;
		cksum += (addr>>8)&0xff;
		cksum += (addr>>16)&0xff;
		break;
	case 3:
		cksum = l+5;
		Bprint(&stdout, "S3%.2lX%.8lluX", l+5, addr);
		cksum += addr&0xff;
		cksum += (addr>>8)&0xff;
		cksum += (addr>>16)&0xff;
		cksum += (addr>>24)&0xff;
		break;
	}

	for(i = 0; i < l; i++) {
		cksum += *s;
		Bprint(&stdout, "%.2X", *s++);
	}
	Bprint(&stdout, "%.2luX\n", (~cksum)&0xff);
	addr += l;
}

void
usage(void)
{
	fprint(2, "usage: ms2 [-dsbh] [-a address] [-p pagesize] ?.out\n");
	exits("usage");
}
