#include <u.h>
#include <libc.h>
#include <libg.h>

int	bdown(Mouse*);
void	bsline(void);
void	clearline(void);
void	incrline(Rune);
int	look(int, char*, char*, int);
void	putline(void);
void	showline(void);
void	showpanel(void);
void	showrune(void);

#define	CMARG	1

int	debug;
char *	fname;
int	ufd;
char *	uname = "/lib/unicode";
int	spacewidth;
int	dxy;
int	base;
int	rune;
Point	pchar;
Point	porg;
Point	pline;
char *	sname = "/dev/snarf";
int	sfd;
char	lbuf[256];

/*
 * The following table is abstracted from
 *	The Unicode Standard
 *	Worldwide Character Encoding
 *	Version 1.0, Volume 1
 *
 *	[Thanks to td.]
 */

char *pagename[256]={
	[0x00]	"Control/ASCII/Control/Latin1",
	[0x01]	"European Latin/Extended Latin",
	[0x02]	"Standard Phonetic/Modifier Letters",
	[0x03]	"Generic Diacritical Marks/Greek",
	[0x04]	"Cyrillic",
	[0x05]	"Armenian/Hebrew",
	[0x06]	"Arabic",
	[0x07]	"Unassigned, Reserved for Right-To-Left Characters",
	[0x08]	"Unassigned, Reserved for Right-To-Left Characters",
	[0x09]	"Devanagari/Bengali",
	[0x0a]	"Gurmukhi/Gujarati",
	[0x0b]	"Oriya/Tamil",
	[0x0c]	"Telugu/Kannada",
	[0x0d]	"Malayalam",
	[0x0e]	"Thai/Lao",
	[0x10]	"Tibetan/Georgian",
	[0x20]	"General Punctuation/Superscripts and Subscripts/Currency Symbols/Diacritical Marks for Symbols",
	[0x21]	"Letterlike Symbols/Number Forms/Arrows",
	[0x22]	"Mathematical Operators",
	[0x23]	"Miscellaneous Technical",
	[0x24]	"Pictures for Control Codes/Optical Character Recognition/Enclosed Alphanumerics",
	[0x25]	"Form and Chart Components/Blocks/Geometric Shapes",
	[0x26]	"Miscellaneous Dingbats",
	[0x27]	"Zapf Dingbats",
	[0x30]	"CJK Symbols and Punctuation/Hiragana/Katakana",
	[0x31]	"Bopomofo/Hangul Elements/CJK Miscellaneous",
	[0x32]	"Enclosed CJK Letters and Ideographs",
	[0x33]	"CJK Squared Words/CJK Squared Abbreviations",
};

void
main(int argc, char **argv)
{
	Event e;
	Point p;
	int b;

	for(b=0x34; b<=0x3d; b++)
		pagename[b] = "Korean Hangul Syllables";
	for(b=0x4e; b<=0x9f; b++)
		pagename[b] = "CJK Ideographs";
	for(b=0xe0; b<=0xf8; b++)
		pagename[b] = "Private Use Area";
	for(b=0xf9; b<=0xff; b++)
		pagename[b] = "Compatibility Zone";

	ARGBEGIN{
	case 's':
		sname = ARGF();
		break;
	case 'u':
		uname = ARGF();
		break;
	case 'D':
		++debug;
		break;
	}ARGEND

	if(argc > 0)
		fname = argv[0];

	ufd = open(uname, OREAD);
	if(ufd < 0)
		fprint(2, "%s: can't open %s: %r\n", argv0, uname);
	sfd = open(sname, OREAD);
	if(sfd >= 0){
		b = read(sfd, lbuf, sizeof lbuf-1);
		close(sfd);
		if(b >= 0)
			lbuf[b] = 0;
	}
	binit(0, fname, argv0);
	dxy = font->height + 2*CMARG;
	spacewidth = strwidth(font, " ");
	rune = -1;
	ereshaped(bscreenrect(0));
	einit(Emouse|Ekeyboard);
	for(;;)switch(event(&e)){
	case Emouse:
		b = bdown(&e.mouse);
		if(b == 0)
			break;
		p = div(sub(e.mouse.xy, porg), dxy);
		if(p.x < 0 || p.x >= 16 || p.y < 0 || p.y >= 16)
			break;
		if(b == 3){
			base = (16*p.x + p.y)<<8;
			rune = -1;
			ereshaped(screen.r);
		}else{
			rune = base + 16*p.x + p.y;
			showrune();
			if(b == 2)
				incrline(rune);
		}
		break;
	case Ekeyboard:
		switch(e.kbdc){
		case 0x04:		/* ctl-D */
			exits(0);
			break;
		case 0x08:		/* bs */
			bsline();
			break;
		case 0x15:		/* ctl-U */
			clearline();
			break;
		default:
			incrline(e.kbdc);
			break;
		}
		break;
	}
}

void
ereshaped(Rectangle r)
{
	screen.r = r;
	r = inset(r, 5);
	bitblt(&screen, r.min, &screen, r, Zero);
	r.min.x += 16;
	clipr(&screen, r);
	showpanel();
	if(rune >= 0)
		showrune();
	showline();
}

void
showpanel(void)
{
	Point org, p;
	Rune r;
	char buf[16], *t;
	int i, j, n;

	org = screen.clipr.min;
	sprint(buf, "%.2uxxx ", base>>8);
	p = string(&screen, org, font, buf, S);
	if(t = pagename[base>>8])	/* assign = */
		string(&screen, p, font, t, S);
	org.y += font->height;
	pchar = org;
	org = add(org, Pt(2*spacewidth, (3*dxy)/2));
	for(i=0; i<16; i++){
		sprint(buf, "%x", i);
		p = add(org, mul(Pt(i, 0), dxy));
		string(&screen, p, font, buf, S);
		p.y += (3*dxy)/2;
		r = base + 16*i;
		for(j=0; j<16; j++, r++, p.y+=dxy){
			n = runetochar(buf, &r);
			buf[n] = 0;
			string(&screen, p, font, buf, S);
			if(i == 0 && j == 0)
				porg = p;
		}
	}
	p = add(org, Pt((3*dxy)/2 + 15*dxy, (3*dxy)/2));
	for(j=0; j<16; j++, p.y+=dxy){
		sprint(buf, "%x", j);
		string(&screen, p, font, buf, S);
	}
	pline.x = screen.clipr.min.x;
	pline.y = org.y + 18*dxy;
}

void
showrune(void)
{
	Rectangle r;
	Point p;
	char key[8], line[768];

	r.min = pchar;
	r.max.x = screen.clipr.max.x;
	r.max.y = pchar.y+font->height;
	bitblt(&screen, pchar, &screen, r, Zero);
	sprint(key, "%.4ux ", rune);
	p = string(&screen, pchar, font, key, S);
	key[4] = 0;
	if(ufd>=0 && look(ufd, key, line, sizeof line) && strlen(line)>5)
		string(&screen, p, font, line+5, S);
}

void
showline(void)
{
	Rectangle r;

	r.min = pline;
	r.max.x = screen.clipr.max.x;
	r.max.y = pline.y+font->height;
	bitblt(&screen, pline, &screen, r, Zero);
	if(lbuf[0])
		string(&screen, pline, font, lbuf, S);
}

void
putline(void)
{
	int n;

	sfd = create(sname, OWRITE, 0666);
	if(sfd < 0)
		return;
	n = strlen(lbuf);
	if(n > 0)
		write(sfd, lbuf, n);
	close(sfd);
	showline();
}

void
clearline(void)
{
	lbuf[0] = 0;
	putline();
}

void
bsline(void)
{
	int c, n;

	n = strlen(lbuf);
	while(n > 0){
		c = *(uchar *)&lbuf[--n];
		lbuf[n] = 0;
		if((c & 0xc0) != 0x80)
			break;
	}
	putline();
}

void
incrline(Rune r)
{
	int n;

	if(r == 0)
		return;
	n = strlen(lbuf);
	if(n+UTFmax+1 >= sizeof lbuf)
		return;
	n += runetochar(lbuf+n, &r);
	lbuf[n] = 0;
	putline();
}

int
bdown(Mouse *m)
{
	static int ob;
	int b = 0;

	if(m->buttons == ob)
		return 0;
	if((m->buttons&1) && !(ob&1))
		b |= 1;
	if((m->buttons&2) && !(ob&2))
		b |= 2;
	if((m->buttons&4) && !(ob&4))
		b |= 4;
	ob = m->buttons;
	switch(b){
	case 1:	return 1;
	case 2:	return 2;
	case 4:	return 3;
	}
	return 0;
}

/*
 * Return the offset of the first byte of the line containing byte offs in file fd.
 * Fill buf with the line.
 */
int
line(int offs, int fd, char *buf, int nbuf)
{
	char *nl;
	int n, nread;

	nread=nbuf-1;		/* save space to add a nul */
	while(offs){
		if(offs < nread){
			nread = offs;
			offs = 0;
		}else
			offs -= nread;
		seek(fd, offs, 0);
		n=read(fd, buf, nread);
		if(n<=0)
			return -1;		/* should never happen */
		buf[n] = 0;
		if(nl = strrchr(buf, '\n')){	/* assign = */
			offs += (nl-buf)+1;
			break;
		}
	}
	seek(fd, offs, 0);
	n=read(fd, buf, nbuf-1);
	if(n<=0)
		return -1;	/* Really(!) should never happen */
	buf[n] = 0;
	if(nl = strchr(buf, '\n'))	/* assign = */
		*nl = 0;
	return offs;
}

int
look(int fd, char *key, char *out, int nout)
{
	long lwb, mid, upb;
	Dir d;
	int nkey;

	if(nout<2)
		return 0;		/* need at least space for \n\0 */
	if(dirfstat(fd, &d) < 0)
		return 0;
	nkey = strlen(key);
	lwb = line(0, fd, out, nout);
	if(lwb < 0)
		return 0;
	switch(strncmp(out, key, nkey)){
	case -1:break;
	case 0:	return 1;
	case 1:	return 0;
	}
	upb=line(d.length-1, fd, out, nout);
	if(upb < 0)
		return 0;
	switch(strncmp(out, key, nkey)){
	case -1:return 0;
	case 0:	return 1;
	case 1:	break;
	}
	while(lwb<=upb){
		mid = line((lwb+upb)/2, fd, out, nout);
		if(mid<0)
			return 0;
		switch(strncmp(out, key, nkey)){
		case -1: lwb=mid+strlen(out)+1; break;
		case  0: return 1;
		case  1: upb=mid-1; break;
		}
	}
	return 0;
}
