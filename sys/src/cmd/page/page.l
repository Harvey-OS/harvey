#include <alef.h>
#include <bio.h>
#include <libg.h>

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

aggr Tab
{
	usint	run;
	usint	bits;
	int	code;
};

aggr Input
{
	byte	*name;
	int	fd;
};

Tab	wtab[8192];
Tab	btab[8192];
byte	bitrev[256];
byte	bitnonrev[256];
Input *input;

int		readrow(byte *rev, int*);
void		initwbtab();
void		sync(byte*);
Bitmap	*readfile(Input*, byte*);
int		track(Bitmap*);
void		mouseproc();
void		kbdproc();

int		nbytes;
byte		*bytes;
byte		*pixels;
byte		*buf;
int		y;
uint		bitoffset;
uint		word24;
int		mousepid;
int		mainpid;
int		kbdpid;
int		rflag = 0;
int		readahead = 1;
chan(Mouse) cmouse;
chan(int) ckbd;

enum
{
	Bytes	= 1024*1024,
	Lines	= 1410,	/* 1100 for A4, 1410 for B4 */
	Dots		= 1728,
};

Menu menu;

Cursor reading={
	{-1, -1},
	{0xff, 0x80, 0xff, 0x80, 0xff, 0x00, 0xfe, 0x00, 
	 0xff, 0x00, 0xff, 0x80, 0xff, 0xc0, 0xef, 0xe0, 
	 0xc7, 0xf0, 0x03, 0xf0, 0x01, 0xe0, 0x00, 0xc0, 
	 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, },
	{0x00, 0x00, 0x7f, 0x00, 0x7e, 0x00, 0x7c, 0x00, 
	 0x7e, 0x00, 0x7f, 0x00, 0x6f, 0x80, 0x47, 0xc0, 
	 0x03, 0xe0, 0x01, 0xf0, 0x00, 0xe0, 0x00, 0x40, 
	 0x00, 0x00, 0x01, 0xb6, 0x01, 0xb6, 0x00, 0x00, }
};

QLock	graphics;

void
error(byte *s, ...)
{
	byte buf[256];
	int pid;

	if(s){
		doprint(buf, buf+sizeof buf+1, s, ...);
		fprint(2, "page: %s\n", buf);
	}
	pid = getpid();
	if(pid != mainpid)
		postnote(PNPROC, mainpid, "kill");
	if(pid != mousepid)
		postnote(PNPROC, mousepid, "kill");
	if(pid != kbdpid)
		postnote(PNPROC, kbdpid, "kill");
	exits(s);
}

void*
emalloc(int n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		error("malloc fail: %r");
	return p;
}

void*
erealloc(void *p, int n)
{
	p = realloc(p, n);
	if(p == nil)
		error("realloc fail: %r");
	return p;
}

void*
estrdup(byte *s)
{
	byte *p;

	p = strdup(s);
	if(p == nil)
		error("strdup fail: %r");
	return p;
}

void
gcursorswitch(Cursor *c)
{
	graphics.lock();
	cursorswitch(c);
	graphics.unlock();
}

int nargc;
byte **nargv;
byte argbuf[256];

void
cleantmp(void *, byte *s)
{
	int i;
	byte buf[256];

	notify(nil);
	for(i=1; i<nargc; i++)
		remove(nargv[i]);
	if(argbuf[0])
		for(i=1; ; i++){
			sprint(buf, argbuf, i);
			if(remove(buf) != 0)
				break;
		}
	if(strcmp(s, "interrupt") != 0)
		noted(NDFLT);
	exits(s);
}

void
rungs(byte *arg)
{
	byte buf[256];

	sprint(buf, "-sOUTPUTFILE=%s", argbuf);
	execl("/bin/gs", "-sDEVICE=dfaxlow", buf, "-dSAFER", "-dNOPAUSE", "-dQUIET", arg, "quit.ps", nil);
}

(int, byte**)
psargs(int argc, byte **argv)
{
	int i, j, suf;
	byte *arg, *p;
	byte buf[256];
	Waitmsg wmsg;

	nargc = 1;
	nargv = emalloc((nargc+1)*sizeof(byte*));
	nargv[0] = argv[0];
	notify(cleantmp);
	for(i=1; i<argc; i++){
		arg = argv[i];
		if(access(arg, 0) != 0){
			fprint(2, "page: %s: %r\n", arg);
			continue;
		}
		fprint(2, "page: converting %s from postscript...\n", arg);
		p = utfrrune(arg, '/');
		if(p == nil)
			p = arg;
		else
			p++;
		suf = 0;
		for(;;){
			sprint(argbuf, "/tmp/.%s.%%.3d%c", p, suf);
			sprint(buf, argbuf, 1);
			if(access(buf, 0) < 0)
				break;
			if(suf == 0)
				suf = 'a';
			else
				if(suf++ == 'z'){
					fprint(2, "page: can't create temp file for %s\n", arg);
					continue 2;
				}
		}

		ALEFrfflag &= ~RFNOWAIT;
		proc rungs(arg);
		ALEFrfflag |= RFNOWAIT;

		if(wait(&wmsg)<0 || wmsg.msg[0]!=0){
			if(wmsg.msg[0] == 0)
				errstr(wmsg.msg);
			fprint(2, "page: error running ghostscript %s\n", wmsg.msg);
			cleantmp(nil, wmsg.msg);
		}
		for(j=1; ; j++){
			sprint(buf, argbuf, j);
			if(access(buf, 0) < 0)
				break;
			nargc++;
			nargv = erealloc(nargv, (nargc+1)*sizeof(byte*));
			nargv[nargc-1] = estrdup(buf);
		}
	}
	notify(nil);
	rflag = 1;
	return (nargc, nargv);
}

void
main(int argc, byte **argv)
{
	int a, oa, ac, next, fd;
	Bitmap *page[2];
	byte err[256];
	byte **menus;
	Arg *arg;

	mainpid = getpid();

	if(argc <= 1){
    Usage:
		fprint(2, "usage: page file ...\n");
		exits("usage");
	}

	/* see if first file is postscript and reconfigure accordingly */
	fd = open(argv[1], OREAD);
	if(fd<0 || read(fd, err, 2)!=2){
		fprint(2, "page: %s: %r\n", argv[1]);
		exits("usage");
	}
	close(fd);
	if(err[0]=='%' && err[1]=='!')
		(argc, argv) = psargs(argc, argv);

	/* holdover from old code; there are no args now, but they might return */
	arg = arginit(argc, argv);
	while(ac = argopt(arg))
		switch(ac){
		default:
			goto Usage;
		}

	if(arg->ac == 0)
		goto Usage;

	input = emalloc(arg->ac * sizeof(Input));
	menus = emalloc((1+arg->ac+1+1) * sizeof(byte*));
	menu.item = menus;
	menus[0] = "next";
	for(a=0; a<arg->ac; a++){
		menus[a+1] = emalloc(1+strlen(arg->av[a])+1);
		menus[a+1][0] = ' ';
		strcpy(&menus[a+1][1], arg->av[a]);
		input[a].name = arg->av[a];
		/* if open fails, will get message later */
		if(rflag)
			input[a].fd = open(input[a].name, OREAD|ORCLOSE);
		else
			input[a].fd = -1;
	}
	menus[arg->ac+1] = "quit";

	initwbtab();
	buf = emalloc(1024*1024);

	binit(nil, nil, "page");

	oa = 0;
	page[0] = nil;

	alloc cmouse;
	proc mouseproc();
	alloc ckbd;
	proc kbdproc();

	gcursorswitch(&reading);
	page[1] = readfile(&input[0], err);
	if(page[1] == nil)	/* if first is bad, just quit */
		error(err);

	for(a=0; a<arg->ac; a=next){
		menus[oa+1][0] = ' ';
		menus[a+1][0] = '>';
		oa = a;
		if(page[1]){
			if(page[0]){
				bfree(page[0]);
				page[0] = nil;
			}
			page[0] = page[1];
			page[1] = nil;
			bitblt(&screen, screen.r.min, &screen, screen.r, 0);
			bitblt(&screen, screen.r.min, page[0], page[0]->r, S);
			bflush();
		}
		next = 0;
		par{
			do{
				if(page[0])
					next = track(page[0]);
				if(next == 0)
					next = a+1;
				else
					--next;	/* adjust for 0-index of a */
			}while(next == a);
			if(readahead && a+1<arg->ac){
				gcursorswitch(&reading);
				page[1] = readfile(&input[a+1], err);
				if(page[1] == nil)
					fprint(2, "page: %s\n", err);
				gcursorswitch(nil);
			}else
				gcursorswitch(nil);
		}
		if(next<arg->ac && (next!=a+1 || (page[1]==nil && !readahead))){
			if(!readahead && page[0]!=nil){
				bfree(page[0]);
				page[0] = nil;
			}
			gcursorswitch(&reading);
			page[1] = readfile(&input[next], err);
			if(page[1] == nil)
				fprint(2, "page: %s\n", err);
			gcursorswitch(nil);
		}
	}
	bitblt(&screen, screen.r.min, &screen, screen.r, 0);
	error(nil);
}

void
mouseproc(void)
{
	byte buf[14];
	Mouse m;
	int mousefd;

	mousepid = getpid();
	mousefd = open("/dev/mouse", OREAD);
	if(mousefd < 0)
		error("can't open mouse: %r\n");
	for(;;){
		if(read(mousefd, buf, sizeof buf) != sizeof buf)
			error("can't read mouse: %r\n");
		m.buttons = buf[1];
		m.xy.x = BGLONG(buf+2);
		m.xy.y = BGLONG(buf+6);
		m.msec = BGLONG(buf+10);
		cmouse <-= m;
	}
}

void
kbdproc()
{
	int m, n, consfd, ctlfd;
	byte buf[10];
	Rune r;

	kbdpid = getpid();

	consfd = open("/dev/cons", ORDWR);
	if(consfd<0)
		error("can't open /dev/cons");
	ctlfd = open("/dev/consctl", OWRITE);
	if(ctlfd < 0)
		error("can't open /dev/consctl");
	write(ctlfd, "rawon", 5);
	n = 0;
	for(;;){
		while(n>0 && fullrune(buf, n)){
			m = chartorune(&r, buf);
			n -= m;
			memmove(buf, buf+m, n);
			ckbd <-= r;
		}
		m = read(consfd, buf+n, sizeof buf-n);
		if(m <= 0)
			error("keyboard read error");
		n += m;
	}
}

void
reshape(Bitmap *b)
{
	graphics.lock();
	screen.r = bscreenrect(&screen.clipr);
	bitblt(&screen, screen.r.min, b, b->r, S);
	bflush();
	graphics.unlock();
}

int
track(Bitmap *b)
{
	Mouse m;
	Point lastxy, xy, d, dd, min, max;
	int ret, c;

	d = Pt(0, 0);
    loop:
	do{
		alt{
		case c = <-ckbd:
			if(c=='q' || c==0x04)
				return 10000;
			return 0;	/* new page */
		case 	m = <-cmouse:
			if(m.buttons & 0x80)
				reshape(b);
			break;
		}
	}while(m.buttons == 0);
	graphics.lock();
	if(m.buttons & 4){
		(ret, nil) = menu.hit(3, cmouse, m);
		graphics.unlock();
		if(ret < 0)
			goto loop;
		return ret;
	}
	if(m.buttons & 2){
		graphics.unlock();
		do{
			m = <-cmouse;
			if(m.buttons & 0x80)
			reshape(b);
		}while(m.buttons  & 7);
		return 0;
	}
	if(m.buttons & 1){
		xy = m.xy;
		do{
			lastxy = m.xy;
			m = <-cmouse;
			if(m.buttons & 0x80){
				graphics.unlock();
				reshape(b);
				d = Pt(0, 0);
				goto loop;
			}
			dd = sub(m.xy, xy);
			min = add(screen.r.min, add(d, dd));
			max.x = min.x+Dx(b->r);
			max.y = min.y+Dy(b->r);
			bitblt(&screen, min, b, b->r, S);
			if(m.xy.x < lastxy.x)	/* moved left, clear right */
				bitblt(&screen, Pt(max.x, screen.r.min.y), &screen,
					Rect(max.x, screen.r.min.y, screen.r.max.x, screen.r.max.y), 0);
			else	/* moved right, clear left*/
				bitblt(&screen, screen.r.min, &screen,
					Rect(screen.r.min.x, screen.r.min.y, min.x, screen.r.max.y), 0);
			if(m.xy.y < lastxy.y)	/* moved up, clear down */
				bitblt(&screen, Pt(screen.r.min.x, max.y), &screen,
					Rect(screen.r.min.x, max.y, screen.r.max.x, screen.r.max.y), 0);
			else		/* moved down, clear up */
				bitblt(&screen, screen.r.min, &screen,
					Rect(screen.r.min.x, screen.r.min.y, screen.r.max.x, min.y), 0);
			bflush();
		}while(m.buttons);
		d = add(d, dd);
	}
	graphics.unlock();
	while(m.buttons & 7){
		m = <-cmouse;
		if(m.buttons & 0x80)
			reshape(b);
	}
	goto loop;
	return 0;
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
crackhdr(byte *ap, int *hdr)
{
	byte *p, *q;
	int i;

	p = ap;
	q = p;
	for(i=0; i<8; i++){
		if(*p<'0' || '9'<*p)
			return -1;
		hdr[i] = strtoi(p, &q, 0);
		p = q+1;
	}
	return p-ap;
}

int
isbitmapnum(byte *bytes)
{
	int i;

	if(bytes[11] != ' ')
		return 0;
	for(i=11; --i>=0; )
		if(bytes[i]<'0' || '9'<bytes[i])
			break;
	while(i >= 0)
		if(bytes[i--] != ' ')
			return 0;
	return 1;
}

int
isbitmaphdr(byte *bytes)
{
	int i;

	for(i=0; i<5; i++)
		if(!isbitmapnum(bytes+i*12))
			return 0;
	return 1;
}

Bitmap*
readfile(Input *input, byte *err)
{
	int i, j, t, ws, l, r, lines, ldepth, res;
	byte *rev;
	Rectangle rect;
	int hdr[8];
	Bitmap *page;

	err[0] = 0;
	if(input->fd < 0){
		input->fd = open(input->name, OREAD|rflag);
		if(input->fd < 0){
			sprint(err, "can't open %s: %r", input->name);
			return nil;
		}
	}
	nbytes = read(input->fd, buf, 1024*1024);
	if(rflag)
		seek(input->fd, 0, 0);
	else{
		close(input->fd);
		input->fd = -1;
	}
	if(nbytes==1024*1024 || nbytes<=100){
    bad:
		sprint(err, "file improper size or format: %s", input->name);
		return nil;
	}
	bytes = buf;
	if(isbitmaphdr(bytes)){	/* regular plan 9 bitmap */
		ldepth = atoi(bytes+0*12);
		rect.min.x = atoi(bytes+1*12);
		rect.min.y = atoi(bytes+2*12);
		rect.max.x = atoi(bytes+3*12);
		rect.max.y = atoi(bytes+4*12);
		if(ldepth<0 || 3<ldepth)
			goto bad;
		if(rect.min.x>=rect.max.x || rect.min.y>=rect.max.y)
			goto bad;
		bytes += 5*12;
		nbytes -= 5*12;
		graphics.lock();
		page = balloc(rect, ldepth);
		if(page == nil){
			graphics.unlock();
	Nomem:
			readahead = 0;	/* suppress readahead: memory is tight */
			sprint(err, "can't allocate bitmap file %s: %r", input->name);
			return nil;
		}
		ws = 8>>ldepth;	/* pixels per byte */
		/* set l to number of bytes of data per scan line */
		if(rect.min.x >= 0)
			l = (rect.max.x+ws-1)/ws - rect.min.x/ws;
		else{		/* make positive before divide */
			t = (-rect.min.x)+ws-1;
			t = (t/ws)*ws;
			l = (t+rect.max.x+ws-1)/ws;
		}
		if(l*Dy(rect) != nbytes){
			bfree(page);
			graphics.unlock();
			goto bad;
		}
		wrbitmap(page, rect.min.y, rect.max.y, bytes);
		graphics.unlock();
		return page;
	}
	
	if(bytes[0]=='I' && bytes[1]=='I' && bytes[2]=='*'){
		/* dumb PC format */
		bytes += 0xf3;
		nbytes -= 0xf3;
		rev = bitrev;
		memmove(hdr, defhdr, sizeof defhdr);
	}else if(bytes[0]=='\0' && strcmp(bytes+1, "PC Research, Inc")==0){
		/* output from gs device dfaxlow */
		res = 0;
		if(bytes[0x1d])
			res = 1;
		bytes += 0x40;
		nbytes -= 0x40;
		rev = bitnonrev;
		memmove(hdr, defhdr, sizeof defhdr);
		hdr[Hvres] = res;
	}else{
		/* should be a PIC file */
		while(nbytes > 2){
			if(bytes[0]=='\n'){
				if(strncmp(bytes+1, "FDCS=", 5) == 0){
					i = crackhdr(bytes+6, hdr);
					if(i < 0){
						sprint(err, "bad FDCS in header: %s", input->name);
						return nil;
					}
					if(hdr[Hwidth] != 0){
						sprint(err, "unsupported width: %s", input->name);
						return nil;
					}
					if(hdr[Hcomp] != 0){
						sprint(err, "unsupported compression: %s", input->name);
						return nil;
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
	graphics.lock();
	page = balloc(Rect(0, 0, Dots/2, Lines), screen.ldepth!=0);
	graphics.unlock();
	if(page == nil)
		goto Nomem;
	bitoffset = 24;
	word24 = 0;
	sync(rev);
	lines = Lines;
	if(hdr[Hvres] == 1)
		lines *= 2;
	if(screen.ldepth == 0){
		if(pixels == nil)
			pixels = emalloc((Dots/(8*2)) * Lines);
		memset(pixels, 0, (Dots/(8*2)) * Lines);
	}else{
		if(pixels == nil)
			pixels = emalloc((Dots/8) * Lines);
		memset(pixels, 0, (Dots/8) * Lines);
	}
	for(y=0; y<lines; y++){
		r = readrow(rev, hdr);
		if(r < 0)
			break;
		if(r == 0)
			sync(rev);
	}
	if(hdr[Hvres] == 1)
		y /= 2;
	if(y < 100){
		graphics.lock();
		bfree(page);
		graphics.unlock();
		goto bad;
	}
	/* send in chunks to enable parallelism */
	for(i=0; i<y; i+=100){
		j = i+100;
		if(j > y)
			j = y;
		graphics.lock();
		if(screen.ldepth == 0)
			wrbitmap(page, i, j, pixels+i*(Dots/(8*2)));
		else
			wrbitmap(page, i, j, pixels+i*(Dots/8));
		graphics.unlock();
	}
	return page;
}

int
readrow(byte *rev, int *hdr)
{
	int bo, state;
	Tab *tab, *t;
	int x, oldx, x2, oldx2, dx, xx;
	uint w24;
	byte *p, *q;

	state = White;
	oldx = 0;
	bo = bitoffset;
	w24 = word24;
	x = y;
	if(hdr[Hvres] == 1)	/* high resolution */
		x /= 2;
	if(screen.ldepth == 0)
		p = pixels + x*(Dots/(8*2));
	else
		p = pixels + x*(Dots/8);
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
		else if(screen.ldepth == 0){
			oldx2 = oldx/2;
			x2 = x/2;
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
		}else{
			/* it's really two bits per pixel, but act like it's 1728 one-bit pixels */
			/* this generates funny grey scale */
			xx = oldx&7;
			q = p+oldx/8;
			if(x/8 == oldx/8)	/* all in one byte */
				*q |= (0xFF>>xx) & (0xFF<<(8-(x&7)));
			else{
				dx = x - oldx;
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
	return 0;
}


void
sync(byte *rev)
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

aggr File
{
	byte	*val;
	int	code;
};

File ibtab[] = {
#include "btab"
{nil, 0}
};

File iwtab[] = {
#include "wtab"
{nil, 0}
};

int
binary(byte *s)
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
	byte *b;

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
			fprint(2, "page: oops1 l = %d %s\n", l, b);

		v = v<<(13-l);
		for(j=0; j<(1<<((13-l))); j++) {
			if(tab[v].code != ERR)
				fprint(2, "page: oops2 %d %s\n", r, b);
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
			if(r >= 64)
				tab[v].code = MAKE;
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
initwbtab()
{
	tabinit(iwtab, wtab);
	tabinit(ibtab, btab);
}
