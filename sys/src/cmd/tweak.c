#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>

/*
 * This program keeps the system's and the application's bitmap and
 * subfont structures consistent, but that is really not necessary;
 * in fact it isn't even necessary to hold the subfonts in the system
 * at all.  Doing it this way makes it easy to do I/O on subfonts.
 * Even so, edits to individual character's Fontchars are not passed
 * to the system.
 */

typedef struct	Thing	Thing;

struct Thing
{
	Bitmap	*b;
	Subfont *s;
	char	*name;		/* file name */
	int	face;		/* is 48x48 face file or cursor file*/
	Rectangle r;		/* drawing region */
	Rectangle tr;		/* text region */
	Rectangle er;		/* entire region */
	long	c;		/* character number in subfont */
	int	mod;		/* modified */
	int	mag;		/* magnification */
	Rune	off;		/* offset for subfont indices */
	Thing	*parent;	/* thing of which i'm an edit */
	Thing	*next;
};

enum
{
	Border	= 1,
	Up	= 1,
	Down	= 0,
	Mag	= 4,
	Maxmag	= 10,
};

enum
{
	Mopen,
	Mread,
	Mwrite,
	Mcopy,
	Mchar,
	Mclose,
	Mexit,
};


char	*menu3str[] = {
	[Mopen]		"open",
	[Mread]		"read",
	[Mwrite]	"write",
	[Mcopy]		"copy",
	[Mchar]		"char",
	[Mclose]	"close",
	[Mexit]		"exit",
	0,
};

Menu	menu3 = {
	menu3str
};

Cursor sweep0 = {
	{-7, -7},
	{0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0,
	 0x03, 0xC0, 0x03, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0xC0, 0x03, 0xC0,
	 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0},
	{0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
	 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x7F, 0xFE,
	 0x7F, 0xFE, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
	 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00}
};

Cursor box = {
	{-7, -7},
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F,
	 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	{0x00, 0x00, 0x7F, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE,
	 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E,
	 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E,
	 0x7F, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE, 0x00, 0x00}
};

Cursor sight = {
	{-7, -7},
	{0x1F, 0xF8, 0x3F, 0xFC, 0x7F, 0xFE, 0xFB, 0xDF,
	 0xF3, 0xCF, 0xE3, 0xC7, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0xE3, 0xC7, 0xF3, 0xCF,
	 0x7B, 0xDF, 0x7F, 0xFE, 0x3F, 0xFC, 0x1F, 0xF8,},
	{0x00, 0x00, 0x0F, 0xF0, 0x31, 0x8C, 0x21, 0x84,
	 0x41, 0x82, 0x41, 0x82, 0x41, 0x82, 0x7F, 0xFE,
	 0x7F, 0xFE, 0x41, 0x82, 0x41, 0x82, 0x41, 0x82,
	 0x21, 0x84, 0x31, 0x8C, 0x0F, 0xF0, 0x00, 0x00,}
};

Cursor busy = {
	{-7, -7},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	 0x00, 0x00, 0x00, 0x0c, 0x00, 0x8e, 0x1d, 0xc7,
	 0xff, 0xe3, 0xff, 0xf3, 0xff, 0xff, 0x7f, 0xfe, 
	 0x3f, 0xf8, 0x17, 0xf0, 0x03, 0xe0, 0x00, 0x00,},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	 0x00, 0x00, 0x00, 0x08, 0x00, 0x04, 0x00, 0x82,
	 0x04, 0x41, 0xff, 0xe1, 0x5f, 0xf1, 0x3f, 0xfe, 
	 0x17, 0xf0, 0x03, 0xe0, 0x00, 0x00, 0x00, 0x00,}
};

Cursor skull = {
	{-7,-7},
	{0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0xe7, 0xe7, 
	 0xff, 0xff, 0xff, 0xff, 0x3f, 0xfc, 0x1f, 0xf8, 
	 0x0f, 0xf0, 0x3f, 0xfc, 0xff, 0xff, 0xff, 0xff, 
	 0xef, 0xf7, 0xc7, 0xe3, 0x00, 0x00, 0x00, 0x00,},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x03,
	 0xE7, 0xE7, 0x3F, 0xFC, 0x0F, 0xF0, 0x0D, 0xB0,
	 0x07, 0xE0, 0x06, 0x60, 0x37, 0xEC, 0xE4, 0x27,
	 0xC3, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}
};

char	*fns[] = {
	"0",      "Zero",		0,
	"~(S|D)", "~(D|S)", "DnorS",	0,
	"~S&D",   "D&~S",   "DandnotS",	0,
	"~S",     "notS",		0,
	"S&~D",   "~D&S",   "notDandS",	0,
	"~D",     "notD",		0,
	"S^D",    "D^S",    "DxorS",	0,
	"~(S&D)", "~(D&S)", "DnandS",	0,
	"S&D",    "D&S",    "DandS",	0,
	"~(S^D)", "~(D^S)", "DxnorS",	0,
	"D",				0,
	"~S|D",   "D|~S",   "DornotS",	0,
	"S",				0,
	"S|~D",   "~D|S",   "notDorS",	0,
	"S|D",    "D|S",    "DorS",	0,
	"F",				0,
	0
};

Rectangle	cntlr;		/* control region */
Rectangle	editr;		/* editing region */
Rectangle	textr;		/* text region */
Thing		*thing;
Mouse		mouse;
char		hex[] = "0123456789abcdefABCDEF";
jmp_buf		err;
char		*file;
int		mag;
uint		copyfn = S;
uint		but1fn = S;
uint		but2fn = 0;
Bitmap		*values;
ulong		val = ~0;
uchar		data[8192];

Thing*	tget(char*);
void	mesg(char*, ...);
void	draw(Thing*, int);
void	select(void);
void	menu(void);
void	error(char*);
void	buttons(int);
char	*fntostr(uint);
int	strtofn(char*);
void	drawall(void);
void	tclose1(Thing*);

void
main(int argc, char *argv[])
{
	int i, j;
	Event e;
	Thing *t;

	mag = Mag;
	binit(error, 0, "tweak");
	values = balloc(Rect(0, 0, 256*Maxmag, Maxmag), screen.ldepth);
	if(values == 0)
		berror("can't balloc");
	for(i=0; i<256; i++)
		for(j=0; j<Maxmag; j++)
			segment(values, Pt(i*Maxmag, j), Pt((i+1)*Maxmag, j), i, S);
	einit(Emouse|Ekeyboard);
	ereshaped(screen.r);
	i = 1;
	setjmp(err);
	for(; i<argc; i++){
		file = argv[i];
		t = tget(argv[i]);
		if(t)
			draw(t, 1);
		bflush();
	}
	file = 0;
	setjmp(err);
	for(;;)
		switch(event(&e)){
		case Ekeyboard:
			break;
		case Emouse:
			mouse = e.mouse;
			if(mouse.buttons & 3){
				select();
				break;
			}
			if(mouse.buttons & 4)
				menu();
		}
}

void
error(char *s)
{
	if(file)
		mesg("can't read %s: %s: %r", file, s);
	else
		mesg("/dev/bitblt error: %s", s);
	if(err[0])
		longjmp(err, 1);
	exits(s);
}

void
redraw(Thing *t)
{
	Thing *nt;
	Point p;

	if(thing==0 || thing==t)
		bitblt(&screen, editr.min, &screen, editr, 0);
	if(thing == 0)
		return;
	if(thing != t){
		for(nt=thing; nt->next!=t; nt=nt->next)
			;
		bitblt(&screen, Pt(screen.r.min.x, nt->er.max.y), &screen,
			Rect(screen.r.min.x, nt->er.max.y, editr.max.x, editr.max.y), 0);
	}
	for(nt=t; nt; nt=nt->next){
		draw(nt, 0);
		if(nt->next == 0){
			p = Pt(editr.min.x, nt->er.max.y);
			bitblt(&screen, p, &screen, Rpt(p, editr.max), 0);
		}
	}
	mesg("");
}

void
ereshaped(Rectangle r)
{
	USED(r);

	screen.r = bscreenrect(&screen.clipr);
	cntlr = inset(screen.clipr, 1);
	editr = cntlr;
	textr = editr;
	textr.min.y = textr.max.y - font->height;
	cntlr.max.y = cntlr.min.y + font->height;
	editr.min.y = cntlr.max.y+1;
	editr.max.y = textr.min.y-1;
	bitblt(&screen, screen.clipr.min, &screen, screen.clipr, 0);
	segment(&screen, Pt(editr.min.x, editr.max.y),
			 Pt(editr.max.x+1, editr.max.y), ~0, F);
	clipr(&screen, editr);
	drawall();
}

void
mesgstr(Point p, int line, char *s)
{
	Rectangle c, r;

	r.min = p;
	r.min.y += line*font->height;
	r.max.y = r.min.y+font->height;
	r.max.x = editr.max.x;
	c = screen.clipr;
	clipr(&screen, r);
	bitblt(&screen, r.min, &screen, r, 0);
	r.min.x++;
	p = string(&screen, r.min, font, s, S);
	r.max.x = p.x + 1;
	r.min.x--;
	bitblt(&screen, r.min, &screen, r, ~D);
	clipr(&screen, c);
	bflush();
}

void
mesg(char *fmt, ...)
{
	char buf[1024];

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	mesgstr(textr.min, 0, buf);
}

void
tmesg(Thing *t, int line, char *fmt, ...)
{
	char buf[1024];

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	mesgstr(t->tr.min, line, buf);
}


void
scntl(char *l)
{
	char vals[16];

	if(val & 0x80000000)
		sprint(vals, "~%lux", ~val);
	else
		sprint(vals, "%lux", val);
	sprint(l, "mag: %d val(hex): %s but1: %s but2: %s copy: %s",
		mag, vals, fntostr(but1fn), fntostr(but2fn), fntostr(copyfn));
}

void
cntl(void)
{
	char buf[256];

	scntl(buf);
	mesgstr(cntlr.min, 0, buf);
}

void
stext(Thing *t, char *l0, char *l1)
{
	Fontchar *fc;
	char buf[256];

	l1[0] = 0;
	sprint(buf, "ldepth:%d r:%d %d  %d %d ", 
		t->b->ldepth, t->b->r.min.x, t->b->r.min.y,
		t->b->r.max.x, t->b->r.max.y);
	if(t->parent)
		sprint(buf+strlen(buf), "mag: %d ", t->mag);
	sprint(l0, "%s file: %s", buf, t->name);
	if(t->c >= 0){
		fc = &t->parent->s->info[t->c];
		sprint(l1, "c(hex): %x c(char): %C x: %d "
			   "top: %d bottom: %d left: %d width: %d iwidth: %d",
			t->c+t->parent->off, t->c+t->parent->off,
			fc->x, fc->top, fc->bottom, fc->left,
			fc->width, Dx(t->b->r));
	}else if(t->s)
		sprint(l1, "offset(hex): %ux n:%d  height:%d  ascent:%d",
			t->off, t->s->n, t->s->height, t->s->ascent);
}

void
text(Thing *t)
{
	char l0[256], l1[256];

	stext(t, l0, l1);
	tmesg(t, 0, l0);
	if(l1[0])
		tmesg(t, 1, l1);
}

void
drawall(void)
{
	Thing *t;

	cntl();
	for(t=thing; t; t=t->next)
		draw(t, 0);
}

int
value(Bitmap *b, int x)
{
	int v, l, w;
	uchar mask;

	l = b->ldepth;
	if(l > 3){
		mesg("ldepth too large");
		return 0;
	}
	w = 1<<l;
	mask = (2<<w)-1;		/* ones at right end of word */
	x -= b->r.min.x&~(7>>l);	/* adjust x relative to first pixel */
	v = data[x>>(3-l)];
	v >>= ((7>>l)<<l) - ((x&(7>>l))<<l);	/* pixel at right end of word */
	v &= mask;			/* pixel at right end of word */
	return v;
}

int
bvalue(int v, int l)
{
	v &= (1<<(1<<l))-1;
	if(l > screen.ldepth)
		v >>= (1<<l) - (1<<screen.ldepth);
	else
		while(l < screen.ldepth){
			v |= v << (1<<l);
			l++;
		}
	return v*Maxmag;
}

void
draw(Thing *nt, int link)
{
	int nl, nf, i, x, y, sx, sy, fdx, dx, dy, v;
	Thing *t;
	Subfont *s;
	Bitmap *b;
	Point p, p1, p2;

	if(link){
		nt->next = 0;
		if(thing == 0){
			thing = nt;
			y = editr.min.y;
		}else{
			for(t=thing; t->next; t=t->next)
				;
			t->next = nt;
			y = t->er.max.y;
		}
	}else{
		if(thing == nt)
			y = editr.min.y;
		else{
			for(t=thing; t->next!=nt; t=t->next)
				;
			y = t->er.max.y;
		}
	}
	s = nt->s;
	b = nt->b;
	nl = font->height;
	if(s || nt->c>=0)
		nl += font->height;
	fdx = Dx(editr) - 2*Border;
	dx = Dx(b->r);
	dy = Dy(b->r);
	if(nt->mag > 1){
		dx *= nt->mag;
		dy *= nt->mag;
		fdx -= fdx%nt->mag;
	}
	nf = 1 + dx/fdx;
	nt->er.min.y = y;
	nt->er.min.x = editr.min.x;
	nt->er.max.x = nt->er.min.x + Border + dx + Border;
	if(nt->er.max.x > editr.max.x)
		nt->er.max.x = editr.max.x;
	nt->er.max.y = nt->er.min.y + Border + nf*(dy+Border);
	nt->r = inset(nt->er, Border);
	nt->er.max.x = editr.max.x;
	bitblt(&screen, nt->er.min, &screen, nt->er, 0);
	for(i=0; i<nf; i++){
		p1 = Pt(nt->r.min.x-1, nt->r.min.y+i*(Border+dy));
		/* draw portion of bitmap */
		p = Pt(p1.x+1, p1.y);
		if(nt->mag == 1)
			bitblt(&screen, p, b,
				Rect(b->r.min.x+i*fdx, b->r.min.y,
				     b->r.max.x+(i+1)*fdx, b->r.max.y), S);
		else{
			for(y=b->r.min.y; y<b->r.max.y; y++){
				sy = p.y+(y-b->r.min.y)*nt->mag;
				rdbitmap(b, y, y+1, data);
				for(x=b->r.min.x+i*(fdx/nt->mag); x<b->r.max.x; x++){
					sx = p.x+(x-i*(fdx/nt->mag)-b->r.min.x)*nt->mag;
					if(sx >= nt->r.max.x)
						break;
					v = bvalue(value(b, x), b->ldepth);
					if(v)
						bitblt(&screen, Pt(sx, sy),
							values,
							Rect(v, 0, v+nt->mag, nt->mag), S);
				}

			}
		}
		/* line down left */
		segment(&screen, p1, Pt(p1.x, p1.y+dy+Border+1), i==0? ~0 : 0, S);
		/* line across top */
		segment(&screen, Pt(p1.x, p1.y-1), Pt(nt->r.max.x+Border, p1.y-1), ~0, F);
		p2 = p1;
		if(i == nf-1)
			p2.x += 1 + dx%fdx;
		else
			p2.x = nt->r.max.x;
		/* line down right */
		segment(&screen, p2, Pt(p2.x, p2.y+dy+1), i==nf-1? ~0 : 0, S);
		/* line across bottom */
		if(i == nf-1){
			p1.y += Border+dy;
			segment(&screen, Pt(p1.x, p1.y-1), Pt(p2.x, p1.y-1), ~0, F);
		}
	}
	nt->tr.min.x = editr.min.x;
	nt->tr.max.x = editr.max.x;
	nt->tr.min.y = nt->er.max.y + Border;
	nt->tr.max.y = nt->tr.min.y + nl;
	nt->er.max.y = nt->tr.max.y + Border;
	text(nt);
}

int
tohex(int c)
{
	if('0'<=c && c<='9')
		return c - '0';
	if('a'<=c && c<='f')
		return 10 + (c - 'a');
	if('A'<=c && c<='F')
		return 10 + (c - 'A');
}

Thing*
tget(char *file)
{
	int i, j, fd, face, x, y, c, ld;
	Bitmap *b, *tb;
	Subfont *s;
	Thing *t;
	Dir d;
	jmp_buf oerr;
	uchar buf[256];
	char *data;

	errstr((char*)buf);	/* flush pending error message */
	memmove(oerr, err, sizeof err);
	if(setjmp(err)){
   Err:
		memmove(err, oerr, sizeof err);
		return 0;
	}
	fd = open(file, OREAD);
	if(fd < 0){
		mesg("can't open %s: %r", file);
		goto Err;
	}
	if(dirfstat(fd, &d) < 0){
		mesg("can't stat bitmap file %s: %r", file);
		close(fd);
		goto Err;
	}
	if(read(fd, buf, 2) != 2){
		mesg("can't read %s: %r", file);
		close(fd);
		goto Err;
	}
	seek(fd, 0, 0);
	if(buf[0]=='0' && buf[1]=='x'){
		/*
		 * face file
		 */
		face = 1;
		s = 0;
		data = malloc(d.length+1);
		if(data == 0){
			mesg("can't malloc buffer: %r");
			close(fd);
			goto Err;
		}
		data[d.length] = 0;
		if(read(fd, data, d.length) != d.length){
			mesg("can't read bitmap file %s: %r", file);
			close(fd);
			goto Err;
		}
		for(y=0,i=0; i<d.length; i++)
			if(data[i] == '\n')
				y++;
		if(y == 0){
	ill:
			mesg("ill-formed face file %s", file);
			close(fd);
			free(data);
			goto Err;
		}
		for(x=0,i=0; (c=data[i])!='\n'; ){
			if(c==',' || c==' ' || c=='\t'){
				i++;
				continue;
			}
			if(c=='0' && data[i+1] == 'x'){
				i += 2;
				continue;
			}
			if(strchr(hex, c)){
				x += 4;
				i++;
				continue;
			}
			goto ill;
		}
		if(x % y)
			goto ill;
		switch(x / y){
		default:
			goto ill;
		case 1:
			ld = 0;
			break;
		case 2:
			ld = 1;
			break;
		case 4:
			ld = 2;
			break;
		case 8:
			ld = 3;
			break;
		}
		b = balloc(Rect(0, 0, y, y), ld);
		if(b == 0){
			mesg("balloc failed file %s: %r", file);
			free(data);
			close(fd);
			goto Err;
		}
		i = 0;
		for(j=0; j<y; j++){
			for(x=0; (c=data[i])!='\n'; ){
				if(strchr(" \t,{}", c)){
					i++;
					continue;
				}
				if(c=='0' && data[i+1] == 'x'){
					i += 2;
					continue;
				}
				if(strchr(hex, c)){
					buf[x++] = (tohex(c)<<4) | tohex(data[i+1]);
					i += 2;
					continue;
				}
			}
			i++;
			wrbitmap(b, j, j+1, buf);
		}
		free(data);
	}else{
		/*
		 * plain bitmap file
		 */
		face = 0;
		s = 0;
		b = rdbitmapfile(fd);
		if(b == 0){
			mesg("can't read bitmap file %s: %r", file);
			close(fd);
			goto Err;
		}
		if(seek(fd, 0, 1) < d.length){
			/* rdsubfontfile drops bitmap; must make a copy */
			tb = balloc(b->r, b->ldepth);
			if(tb == 0){
				mesg("can't balloc bitmap for file %s: %r", file);
				close(fd);
				goto Err;
			}
			bitblt(tb, tb->r.min, b, b->r, S);
			s = rdsubfontfile(fd, tb);
		}
	}
	close(fd);
	t = malloc(sizeof(Thing));
	if(t == 0){
   nomem:
		mesg("malloc failed: %r");
		if(s)
			subffree(s);
		bfree(b);
		goto Err;
	}
	t->name = strdup(file);
	if(t->name == 0){
		free(t);
		goto nomem;
	}
	t->b = b;
	t->s = s;
	t->face = face;
	t->mod = 0;
	t->parent = 0;
	t->c = -1;
	t->mag = 1;
	t->off = 0;
	memmove(err, oerr, sizeof err);
	return t;
}

int
atline(int x, Point p, char *line, char *buf)
{
	char *s, *c, *word, *hit;
	int w, wasblank;
	Rune r;

	wasblank = 1;
	hit = 0;
	word = 0;
	for(s=line; *s; s+=w){
		w = chartorune(&r, s);
		x += charwidth(font, r);
		if(wasblank && r!=' ')
			word = s;
		wasblank = 0;
		if(r == ' '){
			if(x >= p.x)
				break;
			wasblank = 1;
		}
		if(r == ':')
			hit = word;
	}
	if(x < p.x)
		return 0;
	c = utfrune(hit, ':');
	strncpy(buf, hit, c-hit);
	buf[c-hit] = 0;
	return 1;
}

int
attext(Thing *t, Point p, char *buf)
{
	char l0[256], l1[256];

	if(!ptinrect(p, t->tr))
		return 0;
	stext(t, l0, l1);
	if(p.y < t->tr.min.y+font->height)
		return atline(t->r.min.x, p, l0, buf);
	else
		return atline(t->r.min.x, p, l1, buf);
}

int
type(char *buf, char *tag)
{
	Rune r;
	char *p;

	cursorswitch(&busy);
	p = buf;
	for(;;){
		*p = 0;
		mesg("%s: %s", tag, buf);
		r = ekbd();
		switch(r){
		case '\n':
			mesg("");
			cursorswitch(0);
			return p-buf;
		case 0x15:	/* control-U */
			p = buf;
			break;
		case '\b':
			if(p > buf)
				--p;
			break;
		default:
			p += runetochar(p, &r);
		}
	}
	return 0;	/* shut up compiler */
}

void
textedit(Thing *t, char *tag)
{
	char buf[256];
	char *s;
	Bitmap *b;
	Subfont *f;
	Fontchar *fc, *nfc;
	Rectangle r;
	int i, ld, w, c, doredraw, fdx, x;
	Thing *nt;

	buttons(Up);
	if(type(buf, tag) == 0)
		return;
	if(strcmp(tag, "file") == 0){
		for(s=buf; *s; s++)
			if(*s <= ' '){
				mesg("illegal file name");
				return;
			}
		if(strcmp(t->name, buf) != 0){
			if(t->parent)
				t->parent->mod = 1;
			else
				t->mod = 1;
		}
		for(nt=thing; nt; nt=nt->next)
			if(t==nt || t->parent==nt || nt->parent==t){
				free(nt->name);
				nt->name = strdup(buf);
				if(nt->name == 0){
	nomem:
					mesg("malloc failed: %r");
					return;
				}
				text(nt);
			}
		return;
	}
	if(strcmp(tag, "ldepth") == 0){
		if(buf[0]<'0' || '9'<buf[0] || (ld=atoi(buf))<0 || ld>3){
			mesg("illegal ldepth");
			return;
		}
		if(ld == t->b->ldepth)
			return;
		if(t->parent)
			t->parent->mod = 1;
		else
			t->mod = 1;
		for(nt=thing; nt; nt=nt->next){
			if(nt!=t && nt!=t->parent && nt->parent!=t)
				continue;
			b = balloc(nt->b->r, ld);
			if(b == 0){
	nobmem:
				mesg("balloc failed: %r");
				return;
			}
			bitblt(b, b->r.min, nt->b, nt->b->r, S);
			bfree(nt->b);
			nt->b = b;
			if(nt->s){
				b = balloc(nt->b->r, ld);
				if(b == 0)
					goto nobmem;
				bitblt(b, b->r.min, nt->b, nt->b->r, S);
				f = subfalloc(nt->s->n, nt->s->height, nt->s->ascent,
					nt->s->info, b, ~0, ~0);
				if(f == 0){
	nofmem:
					mesg("can't make subfont: %r");
					return;
				}
				subffree(nt->s);
				nt->s = f;
			}
			draw(nt, 0);
		}
		return;
	}
	if(strcmp(tag, "mag") == 0){
		if(buf[0]<'0' || '9'<buf[0] || (ld=atoi(buf))<=0 || ld>Maxmag){
			mesg("illegal magnification");
			return;
		}
		if(t->mag == ld)
			return;
		t->mag = ld;
		redraw(t);
		return;
	}
	if(strcmp(tag, "ascent") == 0){
		if(buf[0]<'0' || '9'<buf[0] || (ld=atoi(buf))<0 || ld>t->s->height){
			mesg("illegal ascent");
			return;
		}
		if(t->s->ascent == ld)
			return;
		t->s->ascent = ld;
		text(t);
		t->mod = 1;
		return;
	}
	if(strcmp(tag, "height") == 0){
		if(buf[0]<'0' || '9'<buf[0] || (ld=atoi(buf))<0){
			mesg("illegal height");
			return;
		}
		if(t->s->height == ld)
			return;
		t->s->height = ld;
		text(t);
		t->mod = 1;
		return;
	}
	if(strcmp(tag, "left")==0 || strcmp(tag, "width") == 0){
		if(buf[0]<'0' || '9'<buf[0] || (ld=atoi(buf))<0){
			mesg("illegal value");
			return;
		}
		fc = &t->parent->s->info[t->c];
		if(strcmp(tag, "left")==0){
			if(fc->left == ld)
				return;
			fc->left = ld;
		}else{
			if(fc->width == ld)
				return;
			fc->width = ld;
		}
		text(t);
		t->parent->mod = 1;
		return;
	}
	if(strcmp(tag, "offset(hex)") == 0){
		if(!strchr(hex, buf[0])){
	illoff:
			mesg("illegal offset");
			return;
		}
		s = 0;
		ld = strtoul(buf, &s, 16);
		if(*s)
			goto illoff;
		t->off = ld;
		text(t);
		for(nt=thing; nt; nt=nt->next)
			if(nt->parent == t)
				text(nt);
		return;
	}
	if(strcmp(tag, "n") == 0){
		if(buf[0]<'0' || '9'<buf[0] || (w=atoi(buf))<=0){
			mesg("illegal n");
			return;
		}
		f = t->s;
		if(w == f->n)
			return;
		doredraw = 0;
	again:
		for(nt=thing; nt; nt=nt->next)
			if(nt->parent == t){
				doredraw = 1;
				tclose1(nt);
				goto again;
			}
		r = t->b->r;
		if(w < f->n)
			r.max.x = f->info[w].x;
		b = balloc(r, t->b->ldepth);
		if(b == 0)
			goto nobmem;
		bitblt(b, b->r.min, t->b, r, S);
		fdx = Dx(editr) - 2*Border;
		if(Dx(t->b->r)/fdx != Dx(b->r)/fdx)
			doredraw = 1;
		bfree(t->b);
		t->b = b;
		b = balloc(r, t->b->ldepth);
		if(b == 0)
			goto nobmem;
		bitblt(b, b->r.min, t->b, r, S);
		nfc = malloc((w+1)*sizeof(Fontchar));
		if(nfc == 0){
			mesg("malloc failed");
			bfree(b);
			return;
		}
		fc = f->info;
		for(i=0; i<=w && i<=f->n; i++)
			nfc[i] = fc[i];
		if(w+1 < i)
			memset(nfc+i, 0, ((w+1)-i)*sizeof(Fontchar));
		x = fc[f->n].x;
		for(; i<=w; i++)
			nfc[i].x = x;
		f = subfalloc(w, f->height, f->ascent, nfc, b, ~0, ~0);
		if(f == 0)
			goto nofmem;
		subffree(t->s);
		f->info = nfc;
		t->s = f;
		if(doredraw)
			redraw(thing);
		else
			draw(t, 0);
		t->mod = 1;
		return;
	}
	if(strcmp(tag, "iwidth") == 0){
		if(buf[0]<'0' || '9'<buf[0] || (w=atoi(buf))<0){
			mesg("illegal iwidth");
			return;
		}
		w -= Dx(t->b->r);
		if(w == 0)
			return;
		r = t->parent->b->r;
		r.max.x += w;
		c = t->c;
		t = t->parent;
		f = t->s;
		b = balloc(r, t->b->ldepth);
		if(b == 0)
			goto nobmem;
		fc = &f->info[c];
		bitblt(b, b->r.min, t->b,
			Rect(t->b->r.min.x, t->b->r.min.y,
			fc[1].x, t->b->r.max.y), S);
		bitblt(b, Pt(fc[1].x+w, b->r.min.y), t->b,
			Rect(fc[1].x, t->b->r.min.y,
			t->b->r.max.x, t->b->r.max.y), S);
		fdx = Dx(editr) - 2*Border;
		doredraw = 0;
		if(Dx(t->b->r)/fdx != Dx(b->r)/fdx)
			doredraw = 1;
		bfree(t->b);
		t->b = b;
		b = balloc(r, t->b->ldepth);
		if(b == 0)
			goto nobmem;
		bitblt(b, b->r.min, t->b, t->b->r, S);
		fc = &f->info[c+1];
		for(i=c+1; i<=f->n; i++, fc++)
			fc->x += w;
		f = subfalloc(f->n, f->height, f->ascent,
			f->info, b, ~0, ~0);
		if(f == 0)
			goto nofmem;
		/* t->s and f share info; free carefully */
		fc = f->info;
		t->s->info = 0;
		subffree(t->s);
		f->info = fc;
		t->s = f;
		if(doredraw)
			redraw(t);
		else
			draw(t, 0);
		/* redraw all affected chars */
		for(nt=thing; nt; nt=nt->next){
			if(nt->parent!=t || nt->c<c)
				continue;
			fc = &f->info[nt->c];
			r.min.x = fc[0].x;
			r.min.y = nt->b->r.min.y;
			r.max.x = fc[1].x;
			r.max.y = nt->b->r.max.y;
			b = balloc(r, nt->b->ldepth);
			if(b == 0)
				goto nobmem;
			bitblt(b, r.min, t->b, r, S);
			doredraw = 0;
			if(Dx(nt->b->r)/fdx != Dx(b->r)/fdx)
				doredraw = 1;
			bfree(nt->b);
			nt->b = b;
			if(c != nt->c)
				text(nt);
			else{
				if(doredraw)
					redraw(nt);
				else
					draw(nt, 0);
			}
		}
		t->mod = 1;
		return;
	}
	mesg("cannot edit %s in file %s", tag, t->name);
}

void
cntledit(char *tag)
{
	char buf[256];
	char *p, *q;
	ulong l;
	int i, inv;

	buttons(Up);
	if(type(buf, tag) == 0)
		return;
	if(strcmp(tag, "mag") == 0){
		if(buf[0]<'0' || '9'<buf[0] || (l=atoi(buf))<=0 || l>Maxmag){
			mesg("illegal magnification");
			return;
		}
		mag = l;
		cntl();
		return;
	}
	if(strcmp(tag, "val(hex)") == 0){
		inv = 0;
		p = buf;
		if(*p == '~'){
			p++;
			inv = 1;
		}
		l = strtoul(p, &q, 16);
		if(l==0 && q==p){
			mesg("illegal magnification");
			return;
		}
		if(inv)
			l = ~l;
		val = l;
		cntl();
		return;
	}
	if(strcmp(tag, "but1")==0
	|| strcmp(tag, "but2")==0
	|| strcmp(tag, "copy")==0){
		i = strtofn(buf);
		if(i < 0){
			mesg("unknown function");
			return;
		}
		if(strcmp(tag, "but1") == 0)
			but1fn = i;
		else if(strcmp(tag, "but2") == 0)
			but2fn = i;
		else
			copyfn = i;
		cntl();
		return;
	}
	mesg("cannot edit %s", tag);
}

void
buttons(int ud)
{
	while((mouse.buttons==0) != ud)
		mouse = emouse();
}

int
sweep(int butmask, Rectangle *r)
{
	Point p;

	cursorswitch(&sweep0);
	buttons(Down);
	if(mouse.buttons != butmask){
		buttons(Up);
		cursorswitch(0);
		return 0;
	}
	p = mouse.xy;
	cursorswitch(&box);
	r->min = p;
	r->max = p;
	while(mouse.buttons == butmask){
		border(&screen, *r, -2, ~D);
		bflush();
		mouse = emouse();
		border(&screen, *r, -2, ~D);
		*r = rcanon(Rpt(p, mouse.xy));
	}
	cursorswitch(0);
	if(mouse.buttons){
		buttons(Up);
		return 0;
	}
	return 1;
}

void
openedit(Thing *t, Point pt, int c)
{
	int x, y;
	Point p;
	Rectangle r;
	Rectangle br;
	Fontchar *fc;
	Thing *nt;

	br = t->b->r;
	if(t->s == 0){
		c = -1; 
		/* if big enough to bother, sweep box */
		if(Dx(br)<=16 && Dy(br)<=16)
			r = br;
		else{
			if(!sweep(1, &r))
				return;
			r = raddp(r, sub(br.min, t->r.min));
			if(!rectclip(&r, br))
				return;
			if(Dx(br) <= 8){
				r.min.x = br.min.x;
				r.max.x = br.max.x;
			}else if(Dx(r) < 4){
	    toosmall:
				mesg("rectangle too small");
				return;
			}
			if(Dy(br) <= 8){
				r.min.y = br.min.y;
				r.max.y = br.max.y;
			}else if(Dy(r) < 4)
				goto toosmall;
		}
	}else if(c >= 0){
		fc = &t->s->info[c];
		r.min.x = fc[0].x;
		r.min.y = br.min.y;
		r.max.x = fc[1].x;
		r.max.y = br.min.y + Dy(br);
	}else{
		/* just point at character */
		fc = t->s->info;
		p = add(pt, sub(br.min, t->r.min));
		x = br.min.x;
		y = br.min.y;
		for(c=0; c<t->s->n; c++,fc++){
	    again:
			r.min.x = x;
			r.min.y = y;
			r.max.x = x + fc[1].x - fc[0].x;
			r.max.y = y + Dy(br);
			if(ptinrect(p, r))
				goto found;
			if(r.max.x >= br.min.x+Dx(t->r)){
				x -= Dx(t->r);
				y += t->s->height;
				if(fc[1].x > fc[0].x)
					goto again;
			}
			x += fc[1].x - fc[0].x;
		}
		return;
	   found:
		r = br;
		r.min.x = fc[0].x;
		r.max.x = fc[1].x;
	}
	nt = malloc(sizeof(Thing));
	if(nt == 0){
   nomem:
		mesg("can't allocate: %r");
		return;
	}
	memset(nt, 0, sizeof(Thing));
	nt->c = c;
	nt->b = balloc(r, t->b->ldepth);
	if(nt->b == 0){
		free(nt);
		goto nomem;
	}
	bitblt(nt->b, r.min, t->b, r, S);
	nt->name = strdup(t->name);
	if(nt->name == 0){
		bfree(nt->b);
		free(nt);
		goto nomem;
	}
	nt->parent = t;
	nt->mag = mag;
	draw(nt, 1);
}

void
ckinfo(Thing *t, Rectangle mod)
{
	int i, j, k, top, bot, n, zero;
	Fontchar *fc;
	Rectangle r;
	Bitmap *b;
	Thing *nt;

	if(t->parent)
		t = t->parent;
	if(t->s==0 || Dy(t->b->r)==0)
		return;
	b = 0;
	/* check bounding boxes */
	fc = &t->s->info[0];
	r.min.y = t->b->r.min.y;
	r.max.y = t->b->r.max.y;
	for(i=0; i<t->s->n; i++, fc++){
		r.min.x = fc[0].x;
		r.max.x = fc[1].x;
		if(!rectXrect(mod, r))
			continue;
		if(b==0 || Dx(b->r)<Dx(r)){
			if(b)
				bfree(b);
			b = balloc(rsubp(r, r.min), t->b->ldepth);
			if(b == 0){
				mesg("can't balloc");
				break;
			}
		}
		bitblt(b, b->r.min, b, b->r, 0);
		bitblt(b, b->r.min, t->b, r, S);
		top = 100000;
		bot = 0;
		n = 2+(Dx(r)/8<<t->b->ldepth);
		for(j=0; j<b->r.max.y; j++){
			memset(data, 0, n);
			rdbitmap(b, j, j+1, data);
			zero = 1;
			for(k=0; k<n; k++)
				if(data[k]){
					zero = 0;
					break;
				}
			if(!zero){
				if(top > j)
					top = j;
				bot = j+1;
			}
		}
		if(top > j)
			top = 0;
		if(top!=fc->top || bot!=fc->bottom){
			fc->top = top;
			fc->bottom = bot;
			for(nt=thing; nt; nt=nt->next)
				if(nt->parent==t && nt->c==i)
					text(nt);
		}
	}
	if(b)
		bfree(b);
}

Point
screenpt(Thing *t, Point realp)
{
	int fdx, n;
	Point p;

	fdx = Dx(editr)-2*Border;
	if(t->mag > 1)
		fdx -= fdx%t->mag;
	p = mul(sub(realp, t->b->r.min), t->mag);
	if(fdx < Dx(t->b->r)*t->mag){
		n = p.x/fdx;
		p.y += n * (Dy(t->b->r)*t->mag+Border);
		p.x -= n * fdx;
	}
	p = add(p, t->r.min);
	return p;
}

Point
realpt(Thing *t, Point screenp)
{
	int fdx, n, dy;
	Point p;

	fdx = (Dx(editr)-2*Border);
	if(t->mag > 1)
		fdx -= fdx%t->mag;
	p.y = screenp.y-t->r.min.y;
	p.x = 0;
	if(fdx < Dx(t->b->r)*t->mag){
		dy = Dy(t->b->r)*t->mag+Border;
		n = (p.y/dy);
		p.x = n * fdx;
		p.y -= n * dy;
	}
	p.x += screenp.x-t->r.min.x;
	p = add(div(p, t->mag), t->b->r.min);
	return p;
}

void
twidpix(Thing *t, Point p, int set)
{
	Bitmap *b;
	int v, c;

	c = but1fn;
	if(!set)
		c = but2fn;
	b = t->b;
	if(!ptinrect(p, b->r))
		return;
	point(b, p, val, c);
	v = bvalue(val, b->ldepth);
	p = screenpt(t, p);
	bitblt(&screen, p, values, Rect(v, 0, v+t->mag, t->mag), c);
}

void
twiddle(Thing *t)
{
	int set;
	Point p, lastp;
	Bitmap *b;
	Thing *nt;
	Rectangle mod;

	if(mouse.buttons!=1 && mouse.buttons!=2){
		buttons(Up);
		return;
	}
	set = mouse.buttons==1;
	b = t->b;
	lastp = add(b->r.min, Pt(-1, -1));
	mod = Rpt(add(b->r.max, Pt(1, 1)), lastp);
	while(mouse.buttons){
		p = realpt(t, mouse.xy);
		if(!eqpt(p, lastp)){
			lastp = p;
			if(ptinrect(p, b->r)){
				for(nt=thing; nt; nt=nt->next)
					if(nt->parent==t->parent || nt==t->parent)
						twidpix(nt, p, set);
				if(t->parent)
					t->parent->mod = 1;
				else
					t->mod = 1;
				if(p.x < mod.min.x)
					mod.min.x = p.x;
				if(p.y < mod.min.y)
					mod.min.y = p.y;
				if(p.x >= mod.max.x)
					mod.max.x = p.x+1;
				if(p.y >= mod.max.y)
					mod.max.y = p.y+1;
			}
		}
		mouse = emouse();
	}
	ckinfo(t, mod);
}

void
select(void)
{
	Thing *t;
	char line[128], buf[128];
	Point p;

	if(ptinrect(mouse.xy, cntlr)){
		scntl(line);
		if(atline(cntlr.min.x, mouse.xy, line, buf)){
			if(mouse.buttons == 1)
				cntledit(buf);
			else
				buttons(Up);
			return;
		}
		return;
	}
	for(t=thing; t; t=t->next){
		if(attext(t, mouse.xy, buf)){
			if(mouse.buttons == 1)
				textedit(t, buf);
			else
				buttons(Up);
			return;
		}
		if(ptinrect(mouse.xy, t->r)){
			if(t->parent == 0){
				if(mouse.buttons == 1){
					p = mouse.xy;
					buttons(Up);
					openedit(t, p, -1);
				}else
					buttons(Up);
				return;
			}
			twiddle(t);
			return;
		}
	}
}

void
twrite(Thing *t)
{
	int i, j, x, y, fd, ws, ld;
	Biobuf buf;
	Rectangle r;

	if(t->parent)
		t = t->parent;
	cursorswitch(&busy);
	fd = create(t->name, OWRITE, 0666);
	if(fd < 0){
		mesg("can't write %s: %r", t->name);
		return;
	}
	if(t->face){
		r = t->b->r;
		ld = t->b->ldepth;
		/* This heuristic reflects peculiarly different formats */
		ws = 4;
		if(Dx(r) == 16)	/* probably cursor file */
			ws = 1;
		else if(Dx(r)<32 || ld==0)
			ws = 2;
		Binit(&buf, fd, OWRITE);
		for(y=r.min.y; y<r.max.y; y++){
			rdbitmap(t->b, y, y+1, data);
			j = 0;
			for(x=r.min.x; x<r.max.x; j+=ws,x+=ws*8>>ld){
				Bprint(&buf, "0x");
				for(i=0; i<ws; i++)
					Bprint(&buf, "%.2x", data[i+j]);
				Bprint(&buf, ", ");
			}
			Bprint(&buf, "\n");
		}
		Bterm(&buf);
	}else{
		wrbitmapfile(fd, t->b);
		if(t->s)
			wrsubfontfile(fd, t->s);
	}
	t->mod = 0;
	close(fd);
	mesg("wrote %s", t->name);
}

void
tclose1(Thing *t)
{
	Thing *nt;

	if(t == thing)
		thing = t->next;
	else{
		for(nt=thing; nt->next!=t; nt=nt->next)
			;
		nt->next = t->next;
	}
	do
		for(nt=thing; nt; nt=nt->next)
			if(nt->parent == t){
				tclose1(nt);
				break;
			}
	while(nt);
	if(t->s)
		subffree(t->s);
	bfree(t->b);
	free(t->name);
	free(t);
}

void
tclose(Thing *t)
{
	Thing *ct;

	if(t->mod){
		mesg("%s modified", t->name);
		t->mod = 0;
		return;
	}
	/* fiddle to save redrawing unmoved things */
	if(t == thing)
		ct = 0;
	else
		for(ct=thing; ct; ct=ct->next)
			if(ct->next==t || ct->next->parent==t)
				break;
	tclose1(t);
	if(ct)
		ct = ct->next;
	else
		ct = thing;
	redraw(ct);
}

void
tread(Thing *t)
{
	Thing *nt, *new;
	Fontchar *i;
	Rectangle r;
	int nclosed;

	if(t->parent)
		t = t->parent;
	new = tget(t->name);
	if(new == 0)
		return;
	nclosed = 0;
    again:
	for(nt=thing; nt; nt=nt->next)
		if(nt->parent == t){
			if(!rectinrect(nt->b->r, new->b->r)
			|| new->b->ldepth!=nt->b->ldepth){
    closeit:
				nclosed++;
				nt->parent = 0;
				tclose1(nt);
				goto again;
			}
			if((t->s==0) != (new->s==0))
				goto closeit;
			if((t->face==0) != (new->face==0))
				goto closeit;
			if(t->s){	/* check same char */
				if(nt->c >= new->s->n)
					goto closeit;
				i = &new->s->info[nt->c];
				r.min.x = i[0].x;
				r.max.x = i[1].x;
				r.min.y = new->b->r.min.y;
				r.max.y = new->b->r.max.y;
				if(!eqrect(r, nt->b->r))
					goto closeit;
			}
			nt->parent = new;
			bitblt(nt->b, nt->b->r.min, new->b,
				nt->b->r, S);
		}
	new->next = t->next;
	if(t == thing)
		thing = new;
	else{
		for(nt=thing; nt->next!=t; nt=nt->next)
			;
		nt->next = new;
	}
	if(t->s)
		subffree(t->s);
	bfree(t->b);
	free(t->name);
	free(t);
	for(nt=thing; nt; nt=nt->next)
		if(nt==new || nt->parent==new)
			if(nclosed == 0)
				draw(nt, 0);	/* can draw in place */
			else{
				redraw(nt);	/* must redraw all below */
				break;
			}
}

void
tchar(Thing *t)
{
	char buf[256], *p;
	Rune r;
	ulong c, d;

	if(t->s == 0){
		mesg("not a subfont");
		return;
	}
	if(type(buf, "char (hex or character or hex-hex)") == 0)
		return;
	if(utflen(buf) == 1){
		chartorune(&r, buf);
		c = r;
		d = r;
	}else{
		if(!strchr(hex, buf[0])){
			mesg("illegal hex character");
			return;
		}
		c = strtoul(buf, 0, 16);
		d = c;
		p = utfrune(buf, '-');
		if(p){
			d = strtoul(p+1, 0, 16);
			if(d < c){
				mesg("invalid range");
				return;
			}
		}
	}
	c -= t->off;
	d -= t->off;
	while(c <= d){
		if(c<0 || c>=t->s->n){
			mesg("0x%lux not in font %s", c+t->off, t->name);
			return;
		}
		openedit(t, Pt(0, 0), c);
		c++;
	}
}

void
apply(void (*f)(Thing*))
{
	Thing *t;

	cursorswitch(&sight);
	buttons(Down);
	if(mouse.buttons == 4)
		for(t=thing; t; t=t->next)
			if(ptinrect(mouse.xy, t->er)){
				buttons(Up);
				f(t);
				break;
			}
	buttons(Up);
	cursorswitch(0);
}

void
copy(void)
{
	Thing *st, *dt, *nt;
	Rectangle sr, dr, fr;
	Point p1, p2;
	int but, up;

	if(!sweep(4, &sr))
		return;
	for(st=thing; st; st=st->next)
		if(rectXrect(sr, st->r))
			break;
	if(st == 0)
		return;
	/* click gives full rectangle */
	if(Dx(sr)<4 && Dy(sr)<4)
		sr = st->r;
	rectclip(&sr, st->r);
	p1 = realpt(st, sr.min);
	p2 = realpt(st, Pt(sr.min.x, sr.max.y));
	if(p1.x != p2.x){	/* swept across a fold */
   onafold:
		mesg("sweep spans a fold");
		return;
	}
	p2 = realpt(st, sr.max);
	sr.min = p1;
	sr.max = p2;
	fr.min = screenpt(st, sr.min);
	fr.max = screenpt(st, sr.max);
	border(&screen, fr, -3, ~D);
	p1 = sub(p2, p1);	/* diagonal */
	if(p1.x==0 || p1.y==0){
    Return:
		border(&screen, fr, -3, ~D);
		return;
	}
	cursorswitch(&box);
	up = 0;
	for(; mouse.buttons==0; mouse=emouse()){
		for(dt=thing; dt; dt=dt->next)
			if(ptinrect(mouse.xy, dt->er))
				break;
		if(up)
			border(&screen, dr, -2, ~D);
		up = 0;
		if(dt == 0)
			continue;
		dr.max = screenpt(dt, realpt(dt, mouse.xy));
		dr.min = sub(dr.max, mul(p1, dt->mag));
		if(!rectXrect(dr, dt->r))
			continue;
		border(&screen, dr, -2, ~D);
		up = 1;
	}
	/* if up==1, we had a hit */
	cursorswitch(0);
	if(up)
		border(&screen, dr, -2, ~D);
	but = mouse.buttons;
	border(&screen, fr, -3, ~D);
	buttons(Up);
	if(!up || but!=4)
		return;
	dt = 0;
	for(nt=thing; nt; nt=nt->next)
		if(rectXrect(dr, nt->r)){
			if(dt){
				mesg("ambiguous sweep");
				return;
			}
			dt = nt;
		}
	if(dt == 0)
		return;
	p1 = realpt(dt, dr.min);
	p2 = realpt(dt, Pt(dr.min.x, dr.max.y));
	if(p1.x != p2.x)
		goto onafold;
	p2 = realpt(dt, dr.max);
	dr.min = p1;
	dr.max = p2;
	bitblt(dt->b, dr.min, st->b, sr, copyfn);
	if(dt->parent){
		bitblt(dt->parent->b, dr.min, dt->b, dr, S);
		dt = dt->parent;
	}
	draw(dt, 0);
	for(nt=thing; nt; nt=nt->next)
		if(nt->parent==dt && rectXrect(dr, nt->b->r)){
			bitblt(nt->b, dr.min, dt->b, dr, S);
			draw(nt, 0);
		}
	ckinfo(dt, dr);
	dt->mod = 1;
}

void
menu(void)
{
	Thing *t;
	char *mod;
	int sel;
	char buf[256];

	sel = menuhit(3, &mouse, &menu3);
	switch(sel){
	case Mopen:
		if(type(buf, "file")){
			t = tget(buf);
			if(t)
				draw(t, 1);
		}
		break;
	case Mwrite:
		apply(twrite);
		break;
	case Mread:
		apply(tread);
		break;
	case Mchar:
		apply(tchar);
		break;
	case Mcopy:
		copy();
		break;
	case Mclose:
		apply(tclose);
		break;
	case Mexit:
		mod = 0;
		for(t=thing; t; t=t->next)
			if(t->mod){
				mod = t->name;
				t->mod = 0;
			}
		if(mod){
			mesg("%s modified", mod);
			break;
		}
		cursorswitch(&skull);
		buttons(Down);
		if(mouse.buttons == 4){
			buttons(Up);
			exits(0);
		}
		buttons(Up);
		cursorswitch(0);
		break;
	}
}

char*
fntostr(uint f)
{
	char **s;
	int i;

	f &= F;
	s = fns;
	for(i=0; i<f; i++)
		do; while(*s++);
	return *s;
}

int
strtofn(char *str)
{
	char **s;
	int i;

	s = fns;
	i = 0;
	while(i <= F){
		if(*s == 0)
			i++;
		else
			if(strcmp(str, *s) == 0)
				return i;
		s++;
	}
	return -1;
}
