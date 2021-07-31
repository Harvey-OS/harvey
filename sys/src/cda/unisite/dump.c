#include <u.h>
#include <libc.h>
#include <bio.h>

typedef struct Devmask	Devmask;

struct Devmask
{
	int	mask;
	char *	name;
};

Devmask devmasks[] = {
	0x01, "NoHandle",
	0x06, "Site40/48",
	0x02, "Site40",
	0x04, "Site48",
	0x08, "USM-340",
	0xb0, "Chip/Pin/SetSite",
	0xa0, "Pin/SetSite",
	0x90, "Chip/SetSite",
	0x30, "Chip/PinSite",
	0x10, "ChipSite",
	0x20, "PinSite",
	0x40, "Bit6?",
	0x80, "SetSite",
	0, 0
};

#define	SHORT(p)	(((p)[0]<<8)|(p)[1])
#define	LONG(p)		((SHORT(p)<<16)|SHORT((p)+2))

int	dumpit(uchar*, uchar*, int);
int	domanu(uchar*, uchar*, int);
int	dopart(uchar*, uchar*, int);
void	dorec(uchar*, int(*)(uchar*,uchar*,int), int);
void	hout(uchar*, long);
void*	getrec(long);
void	Read(void*, long, long);

extern int	printcol;

int	in;
Biobuf *out;
Biobuf *out2;
int	debug;
int	devices;
int	Hflag;
int	sflag;
uchar *	manuf;

void
main(int argc, char **argv)
{
	uchar buf[4], *rec;
	long start = 0;

	ARGBEGIN{
	case 'H':		/* print no Handler Link support */
		Hflag = 0x01;
		break;
	case '0':		/* show dev's supported by Site40 */
		devices |= 0x02;
		break;
	case '8':		/* show dev's supported by Site48 */
		devices |= 0x04;
		break;
	case 'U':		/* show dev's supported by USM-340 */
		devices |= 0x08;
		break;
	case 'C':		/* show dev's supported by ChipSite */
		devices |= 0x10;
		break;
	case 'P':		/* show dev's supported by PinSite */
		devices |= 0x20;
		break;
	case 'S':		/* show dev's supported by SetSite */
		devices |= 0x80;
		break;
	case 's':		/* don't print support field */
		sflag++;
		break;
	case 'r':		/* dump record at this address */
		start = strtol(ARGF(), 0, 0);
		break;
	case 'A':		/* print alg addresses on fd 2 */
		out2 = malloc(sizeof(Biobuf));
		Binit(out2, 2, OWRITE);
		/* fall through */
	case 'D':
		++debug;
		break;
	}ARGEND

	if(devices == 0)
		devices = 0xff;

	if(argc > 0){
		in = open(argv[0], OREAD);
		if(in < 0){
			perror(argv[0]);
			exits("open");
		}
	}else
		in = 0;
	out = malloc(sizeof(Biobuf));
	Binit(out, 1, OWRITE);

	if(debug){
		Read(buf, 0, 4);
		hout(buf, 4);
		Bprint(out, "\n");
	}
	if(start > 0){
		rec = getrec(start);
		dorec(rec, dumpit, start==4 ? 6 : 18);
	}else{
		rec = getrec(4);
		dorec(rec, domanu, 6);
	}
	free(rec);

	exits(0);
}

int
dumpit(uchar *name, uchar *data, int ndata)
{
	int col, k = 0;

	if(ndata > 6)
		return dopart(name, data, ndata);
	col = printcol+16;
	Bprint(out, "%s\t", name);
	while(printcol < col)
		Bprint(out, "\t");
	hout(data, ndata+k);
	Bprint(out, "\n");
	return k;
}

int
domanu(uchar *name, uchar *data, int ndata)
{
	long addr;
	uchar *rec;
	int col;

	if(debug){
		col = printcol+16;
		Bprint(out, "%s\t", name);
		while(printcol < col)
			Bprint(out, "\t");
		hout(data, ndata);
		Bprint(out, "\n");
	}
	addr = LONG(data);
	if(addr < 4)
		return 0;
	manuf = name;
	rec = getrec(addr);
	dorec(rec, dopart, 18);
	free(rec);
	return 0;
}

int
dopart(uchar *name, uchar *data, int ndata)
{
	long addr0, addr1, family, pinout, devbits, notes;
	int col, j, k = 0;
	Devmask *db;

	if(data[13] == 0x02)	/* is this correct? */
		k = 2;
	if(!debug && name[0] == ' ')
		return k;
	addr0 = LONG(data+0);
	addr1 = LONG(data+4);
	family = SHORT(data+8);
	pinout = SHORT(data+10);
	devbits = data[12];
	notes = data[15];
	if(!(devbits & devices))
		return k;
	devbits &= devices|Hflag;
	if(manuf){
		col = printcol+16;
		Bprint(out, "%s\t", manuf);
		while(printcol < col)
			Bprint(out, "\t");
	}
	col = printcol+16;
	Bprint(out, "%s\t", name);
	while(printcol < col)
		Bprint(out, "\t");
	if(debug){
		Bprint(out, "%8.8ux %8.8ux %4.4ux %4.4ux %2.2ux ",
			addr0, addr1, family, pinout, devbits);
		hout(data+13, ndata+k-13);
		if(out2){
			Bprint(out2, "%8.8ux 0\n", addr0);
			Bprint(out2, "%8.8ux 1\n", addr1);
		}
	}else{
		Bprint(out, "%4.4ux %4.4ux", family, pinout);
		if(!sflag)
			for(j=0,db=devmasks; db->mask; db++){
				if((db->mask&devbits) == db->mask){
					Bprint(out, "%c%s",
						j++?',':'\t', db->name);
					devbits &= ~db->mask;
				}
			}
		if(notes)
			Bprint(out, "\t%d", notes);
	}
	Bprint(out, "\n");
	return k;
}

void
dorec(uchar *buf, int (*f)(uchar*,uchar*,int), int ndata)
{
	uchar *start = buf;
	uchar *ebuf = buf+SHORT(buf);
	uchar *p, *ep;
	int k;

	buf += 2;
	while(buf < ebuf){
		p = buf;
		ep = memchr(buf, 0, ebuf-buf);
		if(ep == 0){
			Bprint(out, "unterminated string at buf+%d\n",
				buf-start);
			fprint(2, "unterminated string at buf+%d\n",
				buf-start);
			return;
		}
		buf = ep + 2 - (ep-p)%2;
		if(buf+ndata > ebuf){
			Bprint(out, "short record at buf+%d\n",
				buf-start);
			fprint(2, "short record at buf+%d\n",
				buf-start);
			return;
		}
		if(debug)
			Bprint(out, "%4.4ux\t", p-start);
		k = (*f)(p, buf, ndata);
		if(k < 0)
			return;
		buf += ndata+k;
	}
}

void
hout(uchar *buf, long nbytes)
{
	if(nbytes > 0)
		Bprint(out, "%2.2ux", *buf++);
	while(--nbytes > 0)
		Bprint(out, " %2.2ux", *buf++);
}

void *
getrec(long offset)
{
	uchar buf[2], *rec;
	long size;

	Read(buf, offset, 2);
	size = SHORT(buf);
	rec = malloc(size);
	if(rec == 0){
		fprint(2, "offset=0x%ux, size=%d, malloc failed\n",
			offset, size);
		exits("malloc");
	}
	if(debug){
		Bprint(out, "offset=0x%ux, size=%d, next=0x%ux\n",
			offset, size, offset+size);
	}
	rec[0]=buf[0], rec[1]=buf[1];
	if(size > 2)
		Read(&rec[2], offset+2, size-2);
	return rec;
}

void
Read(void *buf, long offset, long nbytes)
{
	long nr;

	seek(in, offset, 0);
	nr = read(in, buf, nbytes);
	if(nr != nbytes){
		fprint(2, "offset %d, read %d, expected %d\n",
			offset, nr, nbytes);
		exits("read");
	}
}
