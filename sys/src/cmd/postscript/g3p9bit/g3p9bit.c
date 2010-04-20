#include <u.h>
#include <libc.h>

enum
{
	ERR,
	EOL,
	MAKE,
	TERM,
};

enum
{
	White,
	Black,
};

typedef struct Tab
{
	ushort	run;
	ushort	bits;
	int		code;
} Tab;

Tab	wtab[8192];
Tab	btab[8192];
uchar	bitrev[256];
uchar	bitnonrev[256];

int	readrow(uchar *rev, int*);
void	initwbtab(void);
void	sync(uchar*);
int	readfile(int, char*, char*);

int		nbytes;
uchar	*bytes;
uchar	*pixels;
uchar	*buf;
int		y;
uint		bitoffset;
uint		word24;

enum
{
	Bytes	= 1024*1024,
	Lines	= 1410,	/* 1100 for A4, 1410 for B4 */
	Dots		= 1728,
};

void
error(char *fmt, ...)
{
	char buf[256];
	va_list arg;

	if(fmt){
		va_start(arg, fmt);
		vseprint(buf, buf+sizeof buf, fmt, arg);
		va_end(arg);
		fprint(2, "g3: %s\n", buf);
	}
	exits(fmt);
}

void
usage(void)
{
	fprint(2, "usage: g3p9bit [-gy] file\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int y, fd, n, m;
	char *t;
	char *file, err[ERRMAX], tbuf[5*12+1];
	int gray=0;
	int yscale=1;

	ARGBEGIN{
	case 'g':
		/* do simulated 2bit gray to compress x */
		gray++;
		break;
	case 'y':
		/* double each scan line to double the y resolution */
		yscale=2;
		break;
	default:
		usage();
	}ARGEND

	if(argc > 1)
		usage();

	initwbtab();
	buf = malloc(1024*1024);
	t = malloc((Dots/8)*Lines);
	if(buf==nil || t==nil)
		error("malloc failed: %r\n");
	pixels = (uchar*)t;

	file = "<stdin>";
	fd = 0;
	if(argc > 0){
		file = argv[0];
		fd = open(file, OREAD);
		if(fd < 0)
			error("can't open %s", file);
	}
	y = readfile(fd, file, err);
	if(y < 0)
		error(err);
	sprint(tbuf, "%11d %11d %11d %11d %11d ", gray, 0, 0, Dots/(gray+1), y*yscale);
	write(1, tbuf, 5*12);
	n = (Dots/8)*y*yscale;
	/* write in pieces; brazil pipes work badly with huge counts */
	while(n > 0){
		if(yscale > 1)	/* write one scan line */
			m = Dots/8;
		else{	/* write lots */
			m = n;
			if(m > 8192)
				m = 8192;
		}
		for(y=0; y<yscale; y++){
			if(write(1, t, m) != m)
				error("write error");
			n -= m;
		}
		t += m;
	}
	if(err[0])
		error(err);
	error(nil);
}

enum{
	Hvres,
	Hbaud,
	Hwidth,
	Hlength,
	Hcomp,
	HenabECM,
	HenabBFT,
	Hmsperscan,
};

int	defhdr[8] = {
	0,		/* 98 lpi */
	0,		/* 2400 baud */
	0,		/* 1728 pixels in 215mm */
	0,		/* A4, 297mm */
	0,		/* 1-D modified huffman */
	0,		/* disable ECM */
	0,		/* disable BFT */
	3,		/* 10 ms per scan */
};

int
crackhdr(uchar *ap, int *hdr)
{
	char *p, *q;
	int i;

	p = (char*)ap;
	q = p;
	for(i=0; i<8; i++){
		if(*p<'0' || '9'<*p)
			return -1;
		hdr[i] = strtol(p, &q, 0);
		p = q+1;
	}
	return p-(char*)ap;
}

int
readfile(int f, char *file, char *err)
{
	int i, r, lines;
	uchar *rev;
	int hdr[8];

	err[0] = 0;
	memset(pixels, 0, (Dots/8) * Lines);
	nbytes = readn(f, buf, 1024*1024);
	if(nbytes==1024*1024 || nbytes<=100){
    bad:
		sprint(err, "g3: file improper size or format: %s", file);
		return -1;
	}
	bytes = buf;
	if(bytes[0]=='I' && bytes[1]=='I' && bytes[2]=='*'){	/* dumb PC format */
		bytes += 0xf3;
		nbytes -= 0xf3;
		rev = bitrev;
		memmove(hdr, defhdr, sizeof defhdr);
	}else if(bytes[0] == 0 && strcmp((char*)bytes+1, "PC Research, Inc") == 0){	/* digifax format */
		memmove(hdr, defhdr, sizeof defhdr);
		if(bytes[45] == 0x40 && bytes[29] == 1)	/* high resolution */
			hdr[Hvres] = 1;
		else
			hdr[Hvres] = 0;
		/* hdr[26] | (hdr[27]<<8) is page number */

		bytes += 64;
		nbytes -= 64;
		rev = bitnonrev;
	}else{
		while(nbytes > 2){
			if(bytes[0]=='\n'){
				if(strncmp((char*)bytes+1, "FDCS=", 5) == 0){
					i = crackhdr(bytes+6, hdr);
					if(i < 0){
						sprint(err, "g3: bad FDCS in header: %s", file);
						return -1;
					}
					if(hdr[Hwidth] != 0){
						sprint(err, "g3: unsupported width: %s", file);
						return -1;
					}
					if(hdr[Hcomp] != 0){
						sprint(err, "g3: unsupported compression: %s", file);
						return -1;
					}
					bytes += i+1;
					nbytes -= i+1;
					continue;
				}
				if(bytes[1] == '\n'){
					bytes += 2;
					nbytes -= 2;
					break;
				}
			}
			bytes++;
			nbytes--;
		}
		if(nbytes < 2)
			goto bad;
		rev = bitnonrev;
	}
	bitoffset = 24;
	word24 = 0;
	sync(rev);
	lines = Lines;
	if(hdr[Hvres] == 1)
		lines *= 2;
	for(y=0; y<lines; y++){
		r = readrow(rev, hdr);
		if(r < 0)
			break;
		if(r == 0)
			sync(rev);
	}
	if(hdr[Hvres] == 1)
		y /= 2;
//	if(y < 100)
//		goto bad;
	return y;
}

int
readrow(uchar *rev, int *hdr)
{
	int bo, state;
	Tab *tab, *t;
	int x, oldx, x2, oldx2, dx, xx;
	uint w24;
	uchar *p, *q;

	state = White;
	oldx = 0;
	bo = bitoffset;
	w24 = word24;
	x = y;
	if(hdr[Hvres] == 1)	/* high resolution */
		x /= 2;
	p = pixels + x*Dots/8;
	x = 0;

loop:
	if(x > Dots)
		return 0;
	if(state == White)
		tab = wtab;
	else
		tab = btab;
	if(bo > (24-13)) {
		do {
			if(nbytes <= 0)
				return -1;
			w24 = (w24<<8) | rev[*bytes];
			bo -= 8;
			bytes++;
			nbytes--;
		} while(bo >= 8);
	}

	t = tab + ((w24 >> (24-13-bo)) & 8191);
	x += t->run;
	bo += t->bits;
	if(t->code == TERM){
		if(state == White)
			oldx = x;
		else{
			oldx2 = oldx;
			x2 = x;
			xx = oldx2&7;
			q = p+oldx2/8;
			if(x2/8 == oldx2/8)	/* all in one byte, but if((x2&7)==0), do harder case */
				*q |= (0xFF>>xx) & (0xFF<<(8-(x2&7)));
			else{
				dx = x2 - oldx2;
				/* leading edge */
				if(xx){
					*q++ |= 0xFF>>xx;
					dx -= 8-xx;
				}
				/* middle */
				while(dx >= 8){
					*q++ = 0xFF;
					dx -= 8;
				}
				/* trailing edge */
				if(dx)
					*q |= 0xFF<<(8-dx);
			}
		}
		state ^= White^Black;
		goto loop;
	}
	if(t->code == ERR){
		bitoffset = bo;
		word24 = w24;
		return 0;
	}
	if(t->code == EOL){
		bitoffset = bo;
		word24 = w24;
		return 1;
	}
	goto loop;
}


void
sync(uchar *rev)
{
	Tab *t;
	int c;

	c = 0;
loop:
	if(bitoffset > (24-13)) {
		do {
			if(nbytes <= 0)
				return;
			word24 = (word24<<8) | rev[*bytes];
			bitoffset -= 8;
			bytes++;
			nbytes--;
		} while(bitoffset >= 8);
	}
	t = wtab + ((word24 >> (24-13-bitoffset)) & 8191);
	if(t->code != EOL) {
		bitoffset++;
		c++;
		goto loop;
	}
	bitoffset += t->bits;
}

typedef struct File
{
	char	*val;
	int	code;
}File;

File ibtab[] = {
#include "btab"
{nil, 0}
};

File iwtab[] = {
#include "wtab"
{nil, 0}
};

int
binary(char *s)
{
	int n;

	n = 0;
	while(*s)
		n = n*2 + *s++-'0';
	return n;
}

void
tabinit(File *file, Tab *tab)
{
	int i, j, v, r, l;
	char *b;

	for(v=0; v<8192; v++) {
		tab[v].run = 0;
		tab[v].bits = 1;
		tab[v].code = ERR;
	}
	for(i=0; b=file[i].val; i++){
		l = strlen(b);
		v = binary(b);
		r = file[i].code;
		if(l > 13)
			fprint(2, "g3: oops1 l = %d %s\n", l, b);

		v = v<<(13-l);
		for(j=0; j<(1<<((13-l))); j++) {
			if(tab[v].code != ERR)
				fprint(2, "g3: oops2 %d %s\n", r, b);
			tab[v].run = r;
			tab[v].bits = l;
			tab[v].code = TERM;
			if(r < 0) {
				tab[v].run = 0;
				tab[v].code = EOL;
				if(r < -1) {
					tab[v].bits = 1;
					tab[v].code = MAKE;
				}
			}
			if(r >= 64) {
				tab[v].code = MAKE;
			}
			v++;
		}
	}

	for(i=0; i<256; i++)
		for(j=0; j<8; j++)
			if(i & (1<<j))
				bitrev[i] |= 0x80 >> j;
	for(i=0; i<256; i++)
		bitnonrev[i] = i;
}

void
initwbtab(void)
{
	tabinit(iwtab, wtab);
	tabinit(ibtab, btab);
}
