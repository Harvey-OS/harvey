#include <u.h>
#include <libc.h>
#include <bio.h>

#define	SHORT(p)	(((p)[0]<<8)|(p)[1])
#define	LONG(p)		((SHORT(p)<<16)|SHORT((p)+2))

void	showmsg(int, int);
void	Read(void*, long, long);

extern int	printcol;

char *	fname;
Biobuf *in;
Biobuf *out;
int	debug;
long	offset;
long	maxlen, nfnotes, ncnotes;
uchar *	index;
uchar *	msg;

void
main(int argc, char **argv)
{
	uchar buf[8], *ip;
	long i, addr, len, maxlen;

	ARGBEGIN{
	case 'f':
		fname = ARGF();
		in = Bopen(fname, OREAD);
		if(in == 0){
			perror(fname);
			exits("open");
		}
		break;
	case 'D':
		++debug;
		break;
	}ARGEND

	if(in == 0){
		in = malloc(sizeof(Biobuf));
		Binit(in, 0, OREAD);
	}
	out = malloc(sizeof(Biobuf));
	Binit(out, 1, OWRITE);

	Read(buf, 0, 4);
	nfnotes = SHORT(buf);
	ncnotes = SHORT(buf+2);
	if(debug)
		Bprint(out, "nfnotes=%d, ncnotes=%d\n", nfnotes, ncnotes);
	index = malloc(8*ncnotes);
	Read(index, offset, 8*ncnotes);
	maxlen = 0;
	for(i=0,ip=index; i<ncnotes; i++,ip+=8){
		len = SHORT(ip+4);
		if(len > maxlen)
			maxlen = len;
	}
	if(debug)
		Bprint(out, "maxlen=%d\n", maxlen);
	msg = malloc(maxlen);
	if(debug){
		for(i=0,ip=index; i<ncnotes; i++,ip+=8){
			addr = LONG(ip);
			len = SHORT(ip+4);
			if(len == 0)
				continue;
			Bprint(out, "%4d 0x%8.8ux %d %d\n",
				i+1, addr, len, SHORT(ip+6));
			Read(msg, addr, len);
			Bwrite(out, msg, len);
		}
		exits(0);
	}
	if(argc == 0){
		for(i=1; i<=ncnotes; i++)
			showmsg(i, 0);
	}else{
		while(--argc >= 0)
			showmsg(strtol(*argv++, 0, 10), 1);
	}

	exits(0);
}

void
showmsg(int m, int rflag)
{
	long addr, len;
	int n, f, c, nl;
	uchar *ip; char *sp, *cp;

	if(m <= 0 || m > ncnotes){
Bad:
		fprint(2, "no footnote %d\n", m);
		return;
	}
	ip = &index[8*(m-1)];
	addr = LONG(ip);
	len = SHORT(ip+4);
	if(len == 0)
		goto Bad;
	Read(msg, addr, len);
	if(m > nfnotes && rflag){
		cp = sp = malloc(len);
		memcpy(cp, msg, len);
		n = strtol(cp, &cp, 10);
		while(--n >= 0){
			f = strtol(cp, &cp, 10);
			if(f > 0)
				showmsg(f, 0);
		}
		free(sp);
		return;
	}
	Bprint(out, "%d\t", m);
	ip = msg; nl = 0;
	while(ip<&msg[len]){
		switch(c = *ip++){
		case 0:
		case '\r':
			break;
		case '\n':
			BPUTC(out, c);
			++nl;
			break;
		default:
			if(nl){
				BPUTC(out, '\t');
				nl = 0;
			}
			BPUTC(out, c);
			break;
		}
	}
	for(; nl<2; nl++)
		BPUTC(out, '\n');
}

void
Read(void *buf, long addr, long nbytes)
{
	long nr;

	if(offset != addr){
		if(debug)
			Bprint(out, "seek from 0x%x to 0x%x\n",
				offset, addr);
		Bseek(in, offset=addr, 0);
	}
	nr = Bread(in, buf, nbytes);
	if(nr != nbytes){
		fprint(2, "offset %d, read %d, expected %d\n",
			offset, nr, nbytes);
		exits("read");
	}
	offset += nr;
}
