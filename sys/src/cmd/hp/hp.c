#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#include "cons.h"

#define	XMARGIN	5	/* inset from border of layer */
#define	YMARGIN	5
#define	INSET	3
#define	BUFS	32
#define	HISTSIZ	4096	/* number of history characters */
#define BSIZE	1000

#define	SCROLL	2
#define NEWLINE	1
#define OTHER	0

#define COOKED	0
#define RAW	1

#define	button2()	((mouse.buttons & 07)==2)
#define	button3()	((mouse.buttons & 07)==4)

enum{
	Ehost		= 4,
};

char	*menutext2[] = {
	"backup",
	"forward",
	"reset",
	"clear",
	"send",
	"page",
	0
};

char	*menutext3[] = {
	"24x80",
	"crnl",
	"nl",
	"raw",
	0
};

/* variables associated with the screen */

int	x, y;	/* character positions */
char	*backp;
int	backc;
int	atend;
int	nbacklines;
int	xmax, ymax;
int	blocked;
int	reshape_flag;
int	pagemode;
int	olines;
int	peekc;
Menu	menu2;
Menu	menu3;
char	*histp;
char	hist[HISTSIZ];

/* terminal control */
struct ttystate {
	int	crnl;
	int	nlcr;
} ttystate[2] = { {0, 1}, {0,0} };

/* functions */
void	initialize(int, char **);
void	ebegin(int);
Point	pt(int, int);
void	rectf(Bitmap *, Rectangle, Fcode);
void	xtipple(Rectangle);
void	newline(void);
int	host_avail(void);
int	get_next_char(void);
int	waitchar(void);
int	rcvchar(void);
void	set_input(char *);
void	set_host(Event *);
void	ringbell(void);
int	number(char *);
void	scroll(int,int,int,int);
void	bigscroll(void);
void	readmenu(void);
void	backup(int);
void	ereshaped(Rectangle);
void	reshape(void);
void	sendnchars(int, char *);
void	send_interrupt(void);
int	alnum(int);
void	escapedump(int,uchar *,int);

Font	*font;
Bitmap	*allones;
int	NS;
int	CW;
Consstate *cs;
Rectangle	Drect;
Mouse	mouse;

int	outfd = -1;
Biobuf	*snarffp = 0;

char	*host_buf;
char	*hostp;			/* input from host */
int	host_bsize = 2*BSIZE;
int	hostlength;			/* amount of input from host */
char	echo_input[BSIZE];
char	*echop = echo_input;	/* characters to echo, after canon */
char	sendbuf[BSIZE];	/* hope you can't type ahead more than BSIZE chars */
char	*sendp = sendbuf;

void
main(int argc, char **argv)
{
	char buf[BUFS+1];
	int n;
	int c;
	int standout = 0;
	int insmode = 0;

	initialize(argc, argv);

	for (;;) {
		if (x > xmax || y > ymax) {
			x = 0;
			newline();
		}
		buf[0] = get_next_char();
		buf[1] = '\0';
		switch(buf[0]) {

		case '\000':		/* nulls, just ignore 'em */
			break;

		case '\007':		/* bell */
			ringbell();
			break;

		case '\t':		/* tab modulo 8 */
			x = (x|7)+1;
			break;

		case '\033':
			switch(get_next_char()) {

			case 'j':
				get_next_char();
				break;

			case '&':	/* position cursor &c */
				switch(get_next_char()) {

				case 'a':
					for (;;) {
						n = number(buf);
						switch(buf[0]) {

						case 'r':
						case 'y':
							y = n;
							continue;

						case 'c':
							x = n;
							continue;

						case 'R':
						case 'Y':
							y = n;
							break;

						case 'C':
							x = n;
							break;
						}
						break;
					}
					break;

				case 'd':	/* underline stuff */
					if ((n=get_next_char())>='A' && n <= 'O')
						standout++;
					else if (n == '@')
						standout = 0;
					break;

				default:
					get_next_char();
					break;

				}
				break;

			case 'i':	/* back tab */
				if (x>0)
					x = (x-1) & ~07;
				break;

			case 'H':	/* home cursor */
			case 'h':
				x = 0;
				y = 0;
				break;

			case 'L':	/* insert blank line */
				scroll(y, ymax, y+1, y);
				break;

			case 'M':	/* delete line */
				scroll(y+1, ymax+1, y, ymax);
				break;

			case 'J':	/* clear to end of display */
				xtipple(Rpt(pt(0, y+1),
					    pt(xmax+1, ymax+1)));
				/* flow */
			case 'K':	/* clear to EOL */
				xtipple(Rpt(pt(x, y),
					    pt(xmax+1, y+1)));
				break;

			case 'P':	/* delete char */
				bitblt(&screen, pt(x, y),
					&screen, Rpt(pt(x+1, y),
					pt(xmax+1, y+1)),
				        S);
				xtipple(Rpt(pt(xmax, y),
					    pt(xmax+1, y+1)));
				break;

			case 'Q':	/* enter insert mode */
				insmode++;
				break;

			case 'R':	/* leave insert mode */
				insmode = 0;
				break;

			case 'S':	/* roll up */
				scroll(1, ymax+1, 0, ymax);
				break;

			case 'T':
				scroll(0, ymax, 1, 0);
				break;

			case 'A':	/* upline */
			case 't':
				if (y>0)
					y--;
				if (olines > 0)
					olines--;
				break;

			case 'B':
			case 'w':
				y++;	/* downline */
				break;

			case 'C':	/* right */
			case 'v':
				x++;
				break;

			case 'D':	/* left */
			case 'u':
				x--;

			}
			break;

		case '\b':		/* backspace */
			if(x > 0)
				--x;
			break;

		case '\n':		/* linefeed */
			newline();
			standout = 0;
			if( ttystate[cs->raw].nlcr )
				x = 0;
			break;

		case '\r':		/* carriage return */
			x = 0;
			standout = 0;
			if( ttystate[cs->raw].crnl )
				newline();
			break;

		default:		/* ordinary char */
			n = 1;
			c = 0;
			while (!cs->raw && host_avail() && x+n<=xmax && n<BUFS
			    && (c = get_next_char())>=' ' && c<'\177') {
				buf[n++] = c;
				c = 0;
			}
			buf[n] = 0;
			if (insmode) {
				bitblt(&screen, pt(x+n, y), &screen,
					Rpt(pt(x, y), pt(xmax-n+1, y+1)), S);
			}
			xtipple(Rpt(pt(x,y), pt(x+n, y+1)));
			string(&screen, pt(x, y), font, buf, DxorS);
			if (standout)
				rectf(&screen,
				      Rpt(pt(x,y),pt(x+n,y+1)),
				      DxorS);
			x += n;
			peekc = c;
			break;
		}
	}
}

void
initialize(int argc, char **argv)
{
	rfork(RFENVG|RFNAMEG|RFNOTEG);

	host_buf = malloc(host_bsize);
	hostp = host_buf;
	hostlength = 0;

	binit(0,0,"hp");
	ebegin(Ehost);

	histp = hist;
	menu2.item = menutext2;
	menu3.item = menutext3;
	pagemode = 0;
	blocked = 0;
	NS = font->height ;
	CW = strsize(font, "m").x;

	allones = balloc(Rpt(Pt(0, 0), Pt(64, 64)),screen.ldepth);
	bitblt(allones,Pt(0,0),allones,allones->r,F);

	ereshaped(bscreenrect(0));

	if( argc > 1) {
		sendnchars(strlen(argv[1]),argv[1]);
		sendnchars(1,"\n");
	}
}

void
xtipple(Rectangle r)
{
	rectclip(&r, screen.r);
	rectf(&screen, r, Zero);
}

void
newline(void)
{
	nbacklines--;
	if(y >= ymax) {
		y = ymax;
		if (pagemode && olines >= ymax) {
			blocked = 1;
			return;
		}
		scroll(1, ymax+1, 0, ymax);
	} else
		y++;
	olines++;
}

void
curse(int bl)
{

	rectf(&screen, Rpt(
		sub(pt(x, y),Pt(1,1)),add(pt(x, y), Pt(CW,NS))),DxorS);
	if (bl==0)
		return;
	rectf(&screen, Rpt(pt(x, y),
		    add(pt(x, y), Pt(CW-1,NS-1))), DxorS);
}

int
get_next_char(void)
{
	int c = peekc;
	peekc = 0;
	if (c > 0)
		return(c);
	while (c <= 0) {
		if (backp) {
			c = *backp;
			if (c && nbacklines >= 0) {
				backp++;
				if (backp >= &hist[HISTSIZ])
					backp = hist;
				return(c);
			}
			backp = 0;
		}
		c = waitchar();
	}
	*histp++ = c;
	if (histp >= &hist[HISTSIZ])
		histp = hist;
	*histp = '\0';
	return(c);
}

int
canon(char *ep, int c)
{
	if(c&0200)
		return(SCROLL);
	switch(c) {
		case '\b':
			if( sendp > sendbuf) sendp--;
			*ep++ = '\b';
			*ep++ = ' ';
			*ep++ = '\b';
			break;
		case 0x15:	/* ^U line kill */
			sendp = sendbuf;
			*ep++ = '^';
			*ep++ = 'U';
			*ep++ = '\n';
			break;
		case 0x17:	/* ^W word kill */
			while( sendp > sendbuf && !alnum(*sendp) ) {
				*ep++ = '\b';
				*ep++ = ' ';
				*ep++ = '\b';
				sendp--;
			}
			while( sendp > sendbuf && alnum(*sendp) ) {
				*ep++ = '\b';
				*ep++ = ' ';
				*ep++ = '\b';
				sendp--;
			}
			break;
		case '\177':	/* interrupt */
			sendp = sendbuf;
			send_interrupt();
			return(NEWLINE);
		case '\021':	/* quit */
		case '\r':
		case '\n':
			if( sendp < &sendbuf[512])
				*sendp++ = '\n';
			sendnchars((int)(sendp-sendbuf), sendbuf);
			sendp = sendbuf;
			if( c == '\n' || c == '\r') {
				*ep++ = '\n';
			}
			*ep = 0;
			return(NEWLINE);
		case '\004':	/* EOT */
			if( sendp == sendbuf ) {
				sendnchars(0,sendbuf);
				*ep = 0;
				return(NEWLINE);
			}
			/* fall through */
		default:
			if( sendp < &sendbuf[512])
				*sendp++ = c;
			*ep++ = c;
			break;
		
	}
	*ep = 0;
	return(OTHER);
}

int
waitchar(void)
{
	Event e;
	int c;
	char c2;
	int newmouse;
	int wasblocked;
	int kbdchar = -1;
	char echobuf[3*BSIZE];
	static int lastc = -1;


	for (;;) {
		if(reshape_flag) reshape();
		wasblocked = blocked;
		if (backp)
			return(0);
		if( ecanmouse() && (button2() || button3()) )
			readmenu();
		if( snarffp ) {
			if( (c = Bgetc(snarffp)) < 0) {
				if( lastc != '\n')
					write(outfd,"\n",1);
				Bterm(snarffp);
				snarffp = 0;
				if( lastc != '\n' ) {
					lastc = -1;
					return('\n');
				}
				lastc = -1;
				continue;
			}
			lastc = c;
			c2 = c;
			write(outfd, &c2, 1);
			return(c);
		}
		if (!blocked && host_avail())
			return(rcvchar());
		if(kbdchar > 0) {
			if(blocked)
				reshape();
			if(cs->raw) {
				echobuf[0] = kbdchar;
				sendnchars(1, echobuf);
			} else if(canon(echobuf,kbdchar) == SCROLL) {
				if(!blocked)
					bigscroll();
			} else {
				strcat(echo_input,echobuf);
			}
			blocked = 0;
			kbdchar = -1;
			continue;
		}
		curse(wasblocked);	/* turn on cursor while we're waiting */
		do {
			newmouse = 0;
			switch(eread(blocked ? Emouse|Ekeyboard : 
					       Emouse|Ekeyboard|Ehost, &e)) {
			case Emouse:	mouse = e.mouse;
					if( button2() || button3())
						readmenu();
					else
						newmouse = 1;
					break;
			case Ekeyboard:	kbdchar = e.kbdc; break;
			case Ehost:	set_host(&e); break;
			default:	perror("protocol violation");
					exits("protocol violation");
			}
		} while(newmouse == 1);
		curse(wasblocked);	/* turn cursor back off */
	}
	return 1;		/* to shut up compiler */
}

void
ereshaped(Rectangle r)
{
	screen.r = r;
	reshape_flag = 1;
}

void
exportsize(void)
{
	int	fd;
	char	buf[10];

	if( (fd = create("/env/LINES", OWRITE, 0644)) > 0) {
		sprint(buf,"%d",ymax+1);
		write(fd,buf,strlen(buf));
		close(fd);
	}
	if( (fd = create("/env/COLS", OWRITE, 0644)) > 0) {
		sprint(buf,"%d",xmax+1);
		write(fd,buf,strlen(buf));
		close(fd);
	}
	if( (fd = create("/env/TERM", OWRITE, 0644)) > 0) {
		write(fd,"2621",4);
		close(fd);
	}
}

void
reshape(void)
{
	Drect = inset(screen.r, INSET);
	xmax = (Drect.max.x-Drect.min.x-2*XMARGIN)/CW-1;
	ymax = (Drect.max.y-Drect.min.y-2*YMARGIN)/NS-1;
	if( xmax == 0 || ymax == 0) exits("window gone");
	x = 0;
	y = 0;
	olines = 0;
	exportsize();
	xtipple(Drect);
	reshape_flag = 0;
}

void
readmenu(void)
{
	Rectangle r;

	if( button3() ) {
		menu3.item[1] = ttystate[cs->raw].crnl ? "cr" : "crnl";
		menu3.item[2] = ttystate[cs->raw].nlcr ? "nl" : "nlcr";
		menu3.item[3] = cs->raw ? "cooked" : "raw";

		switch(menuhit(3, &mouse, &menu3)) {
		case 0:		/* 24x80 */
			r.min = screen.r.min;
			r.max = add(screen.r.min,
				Pt(80*CW+2*XMARGIN+2*INSET,
				24*NS+2*YMARGIN+2*INSET));
			border(&screen, r, INSET, F);
			xmax = 79;
			ymax = 23;
			exportsize();
			return;
		case 1:		/* newline after cr? */
			ttystate[cs->raw].crnl = !ttystate[cs->raw].crnl;
			return;
		case 2:		/* cr after newline? */
			ttystate[cs->raw].nlcr = !ttystate[cs->raw].nlcr;
			return;
		case 3:		/* switch raw mode */
			cs->raw = !cs->raw;
			return;
		}
		return;
	}

	menu2.item[5] = pagemode? "scroll": "page";

	switch(menuhit(2, &mouse, &menu2)) {

	case 0:		/* back up */
		if (atend == 0) {
			backc++;
			backup(backc);
		} else {
			rectf(&screen, screen.r, DxorS);
			rectf(&screen, screen.r, DxorS);
		}
		return;

	case 1:		/* move forward */
		backc--;
		if (backc >= 0)
			backup(backc);
		else {
			backc = 0;
			rectf(&screen, screen.r, DxorS);
			rectf(&screen, screen.r, DxorS);
		}
		return;

	case 2:		/* reset */
		backc = 0;
		backup(0);
		return;

	case 3:		/* clear screen */
		ereshaped(screen.r);
		return;

	case 4:		/* send the snarf buffer */
		snarffp = Bopen("/dev/snarf",OREAD);
		return;

	case 5:		/* pause and clear at end of screen */
		pagemode = 1-pagemode;
		if(blocked && !pagemode) {
			ereshaped(screen.r);
			blocked = 0;
		}
		return;
	}
}

void
backup(int count)
{
	register n;
	register char *cp;

	ereshaped(screen.r);
	n = 3*(count+1)*ymax/4;
	cp = histp;
	atend = 0;
	while (n >= 0) {
		cp--;
		if (cp < hist)
			cp = &hist[HISTSIZ-1];
		if (*cp == '\0') {
			atend = 1;
			break;
		}
		if (*cp == '\n')
			n--;
	}
	cp++;
	if (cp >= &hist[HISTSIZ])
		cp = hist;
	backp = cp;
	nbacklines = ymax-2;
}

Point
pt(int x, int y)
{
	extern Point add(Point,Point);
	return add(screen.r.min,
		Pt(x*CW+XMARGIN,y*NS+YMARGIN));
}

void
scroll(int sy, int ly, int dy, int cy)	/* source, limit, dest, which line to clear */
{
	bitblt( &screen, pt(0, dy), &screen, Rpt(pt(0, sy), pt(xmax+1, ly)),
	        S);
	xtipple(Rpt(pt(0, cy), pt(xmax+1, cy+1)));
}

void
bigscroll(void)			/* scroll up half a page */
{
	int half = ymax/3;
	if( x == 0 && y == 0) return;
	if( (y - half) < 0 ) {
		xtipple(Rpt(pt(0,0),pt(xmax+1,ymax+1)));
		x = y = 0;
		return;
	}
	bitblt( &screen, pt(0,0), &screen,
		Rpt(pt(0,half),pt(xmax+1,ymax+1)), S);
	xtipple(Rpt(pt(0,y-half+1),pt(xmax+1,ymax+1)));
	y -= half;
	if(olines) olines -= half;
}

int
number(char *p)
{
	register n = 0;
	register c;

	while ((c = get_next_char()) >= '0' && c <= '9')
		n = n*10 + c - '0';
	*p = c;
	return(n);
}

/* stubs */

void
sendnchars(int n,char *p)
{
	if( write(outfd,p,n) < 0) {
		close(outfd);
		close(0);
		close(1);
		close(2);
		exits("write");
	}
	p[n+1] = 0;
}

void
rectf(Bitmap *b,Rectangle r,Fcode f)
{
	texture(b,r,allones,f);
}

int
host_avail(void)
{
	return( (int)*echop ||
		((hostp - host_buf) < hostlength) );
}

int
rcvchar(void)
{
	int	c;
	if( *echop ) {
		c = (int)*echop++;
		if( !*echop ) {
			echop = echo_input;	
			*echop = 0;
		}
		return(c);
	}
	return((int)*hostp++);
}

void
set_host(Event *e)
{
	hostlength = e->n;
	if( hostlength > host_bsize) {
		host_bsize *= 2;
		host_buf = realloc(host_buf,host_bsize);
	}
	hostp = host_buf;
	memmove(host_buf,e->data,hostlength);
	host_buf[hostlength]=0;
}

void
ringbell(void){}

int
alnum(int c)
{
	if( c >= 'a' && c <= 'z') return(1);
	if( c >= 'A' && c <= 'Z') return(1);
	if( c >= '0' && c <= '9') return(1);
	return(0);
}

void
escapedump(int fd,uchar *str,int len)
{
	int i;
	for(i = 0; i < len; i++) {
		if((str[i] < ' ' || str[i] > '\177') && 
			str[i] != '\n' && str[i] != '\t') fprint(fd,"^%c",str[i]+64);
		else if(str[i] == '\177') fprint(fd,"^$");
		else if(str[i] == '\n') fprint(fd,"^J\n");
		else fprint(fd,"%c",str[i]);
	}
}
