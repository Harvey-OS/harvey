#include	"all.h"

#define	TWOX	1.997
#define TWOY	2.0

/*
 * wire-list post-processor for boards wired with
 * Standard Logic terminal locator.
 *
 * standard input to wraps is output from wrap1.
 *
 * flags are:
 *		two character string in next argument
 *		first and second preference:
 *			0 = increasing x
 *			1 = increasing y
 *			2 = decreasing x
 *			3 = decreasing y
 *	-l	print listing only
 *	-r	set board rotation (default as in board defn.)
 *			anti-clockwise rotation in rt angles
 *				+4 to flip board over
 *	-v	set verbose mode
 *
 *
 * wire wrap commands are as follows:
 *	r	move right a small distance
 *	R	move right a large distance
 *	l	move left a small distance
 *	L	move left a large distance
 *	u	move up a small distance
 *	U	move up a large distance
 *	d	move down a small distance
 *	D	move down a large distance
 *	c	check position of origin
 *	C	check position of origin and corners
 *	snnn	skip to wire number nnn
 *	q	quit the program
 *	?	print details of current wire
 */

int	wfile;		/* for lastwire */

uchar segtab[] =
{
	1|2|4|8|16|32,			/* zero */
	16|32,				/* one */
	1|2|8|16|64,			/* two */
	1|2|4|8|64,			/* three */
	2|4|32|64,			/* four */
	1|4|8|32|64,			/* five */
	4|8|16|32|64,			/* six */
	1|2|4,				/* seven */
	1|2|4|8|16|32|64,		/* eight */
	1|2|4|32|64,			/* nine */
	0,				/* blank */
};
uchar dirc1[] = {
	UR, DR, DASH, 0, 0, 0, UP, 0, UP+DASH
};
uchar dirc2[] = {
	0, 0, 0, UL, DL, DASH, 0, DN, DN+DASH
};

int wirelen[] = {
	 80,   100,  120,  140,  160,  180,  200,  220,
	 240,  260,  280,  300,  320,  340,  360,  380,
	 400,  450,  500,  550,  600,  650,  700,  750,
	 800,  850,  900,  950, 1000, 1050, 1100, 1150,
	1200, 1250, 1300, 1350, 1400, 1450, 1500, 1550,
	0
};

char *	mfile = "/dev/eia0";
int	debug;
int	absx, absy;

int	wireno;		/* total number of wires */
int	rflg;		/* set if user defined rotation */
int	lflg;
int	vflg;
Biobuf	*out;
Biobuf	*fd;		/* temporary file */
int	slot;		/* wire bin slot number */
int	len;		/* wire length */
Pinent	corn[4];	/* four corners */
int	corncnt;	/* number of corners */
int	origin;		/* index for origin corner */
int	lev1no;		/* number of wires at level 1 */
int	lev2no;		/* number of wires at level 2 */
int	startno;	/* seq no first wire */
int	calib;		/* set when board position known */

char *	wroute;
int	rot;
int	xbase, ybase;

char *	wrfile;		/* wrap file */
Biobuf *wr;
int	nlines;
Wbuf	wbuf;		/* wire */
Wbuf	nbuf;		/* next wire */
int	bufset;		/* =1 when wbuf contains info */

int	absx, absy;

Pinent *pptr[2];
char	dirn[2];

void
main(int argc, char **argv)
{
	int cm;
	char *str;

	startno = calib = 0;
	wroute = "01";
	rot = 6;
	xbase = 30000;
	ybase = 30000;
	ARGBEGIN{
	case 'd':
		wroute = ARGF();
		break;
	case 'f':
		mfile = ARGF();
		break;
	case 'l':
		lflg++;
		break;
	case 'r':
		rflg++;
		rot = strtol(ARGF(), 0, 10);
		break;
	case 's':
		break;
	case 'v':
		vflg++;
		break;
	case 'D':
		debug++;
		break;
	default:
		fprint(2, "bad flag %c\n", ARGC());
		exits("usage");
	}ARGEND
	if(argc)
		wrfile = argv[0];
	else
		wrfile = "/fd/0";
	wr = Bopen(wrfile, OREAD);
	if(wr == 0){
		perror(wrfile);
		exits("open");
	}
	if(lflg==0)
		startno = findlast();
	fd = Btmpfile();
	while(wrd() >= 0){
		if(wbuf.stype == 'x'){
			if(!rflg)
				rot = wbuf.mark;
			xbase = wbuf.pent[1].x;
			ybase = wbuf.pent[1].y;
			continue;
		}
		rotate(&wbuf.pent[0]);
		rotate(&wbuf.pent[1]);
		if(wbuf.stype == 'c'){
			if (corncnt < 3){
				corn[corncnt++] = wbuf.pent[0];
				corn[corncnt++] = wbuf.pent[1];
			}
			continue;
		}
		if(wbuf.mark & LEVEL1)
			lev1no++;
		else
			lev2no++;
		Bwrite(fd, &wbuf, sizeof wbuf);
	}
	Bclose(wr);
	Bclose(fd);
	Binit(fd, fd->fid, OREAD);
	out = Bopen("/fd/1", OWRITE);
	if(lflg){
		wireno = 0;
		scan(LEVEL1);
		scan(LEVEL2);
		exits(0);
	}
	bufset = 0;
	wwinit();
	cm = cornset();
	for(;;){
		bufset = 0;
		switch(cm){
		case 0:
			break;
		case 'q':
			print("q\n");
			home();
			exits(0);
		case 'c':
			print("c\n");
			home();
			calib = 0;
			break;
		case 'C':
			print("C\n");
			home();
			cm = cornset();
			continue;
		case -1:
			print("positioning error\n");
			home();
			calib = 0;
			break;
		case 's':
			print("s");
			if(str = rdstr()){	/* assign = */
				startno = strtoul(str, 0, 10);
				if(startno <= (lev1no + lev2no))
					break;
			}
			goto bad;
		case 'v':
			if(vflg++){
				vflg = 0;
				print("quiet mode\n");
			}else
				print("verbose mode\n");
			break;
		default:
		bad:
			print("?\n");
			cm = rdcmd();
			continue;
		}
		if(calib == 0)
			if(cm = findz())	/* assign = */
				continue;
		if(startno <= lev1no){
			wireno = 0;
			print("take wire - level 1\n");
			if(cm = scan(LEVEL1))	/* assign = */
				continue;
		}else
			wireno = lev1no;
		print("take wire - level 2\n");
		if((cm = scan(LEVEL2)) == 0)
			break;
	}
	lastw(0);
	home();
	exits(0);
}



/*
 * one scan to print wire list
 * return zero or command character
 */
int
scan(int lev)
{
	int c;

	Binit(fd, fd->fid, OREAD);
	Bseek(fd, 0, 0);
	while(Bread(fd, &nbuf, sizeof nbuf)){
		if(nbuf.mark & lev){
			if(wireno >= (startno -1)){
				if(lflg)
					list();
				else if(c = install())
					return c;
			}else
				wireno++;
		}
	}
	if(lflg == 0){
		putbin(DASH, DASH);
		return(rdcmd());
	}
	return 0;
}

/*
 * direct installation of one wire
 * return zero or command character
 */

int
install(void)
{
	Pinent *pp;
	int c, nslot, nlen;

	nlen = nbuf.slen;
	nslot = binslot(nlen + STRIP);
	if(nslot >= 0)
		putbin(segtab[nslot >> 3], segtab[nslot & 7]);
	else
		putbin(DASH, DASH);
	if(c = rdcmd())	/* assign = */
		return c;
	wbuf = nbuf;
	bufset = 1;
	wireno++;
	slot = nslot;
	len = nlen;
	pinsort();
	pp = pptr[0];
	if(movptr(pp->x,pp->y) < 0)
		return -1;
	putdirn();
	prwir(vflg);
	Bflush(out);
	startno = wireno;
	lastw(wireno);
	if(c = rdcmd())	/* assign = */
		return c;
	pp = pptr[1];
	if(movptr(pp->x,pp->y) < 0)
		return -1;
	return 0;
}


/*
 * output wire routing info on wire bin
 */
void
putdirn(void)
{
	int i;

	i = (dirn[0] * 3) + dirn[1];
	putbin(dirc1[i], dirc2[i]);
}



/*
 * write to wire-wrap bin from d[]
 * lh character is d[0]
 */
void
putbin(int d0, int d1)
{
	wwcmd(LOADBIN, d1, d0);
}


/*
 * move wire-wrap machine pointer
 * return status after move or -1 if error
 */
int
movptr(int px, int py)
{

	px = px * TWOX;
	py = py * TWOY;
	return wwmove(px - absx, py - absy);
}




/*
 * return bin slot number for wire length w
 */
int
binslot(int w)
{
	int i;

	for(i = 0; wirelen[i] != 0; i++)
		if(w <= wirelen[i])
			return(i);
	print("wire length %d not in bin\n", w);
	return -1;
}


/*
 * print wire list entry
 */
void
list(void)
{
	wireno++;
	wbuf = nbuf;
	pinsort();
	len = nbuf.slen;
	slot = binslot(len + STRIP);
	prwir(1);
}


/*
 * print one wire
 * if !flg print only wire number
 */
void
prwir(int flg)
{
	Bprint(out, "%4d %14s", wireno, wbuf.sname);
	if(flg){
		Bprint(out, "   %5d ", len);
		prpin(pptr[0]);
		prdir();
		prpin(pptr[1]);
		Bprint(out, "  level %c  bin %o",
			(wbuf.mark & LEVEL2)? '2': '1', slot);
	}
	Bprint(out, "\n");
}


/*
 * print one pin for listing
 */
void
prpin(Pinent *pp)
{
	Bprint(out, "%5s-%-5s", pp->chname, pp->pname);
}


void
prpinxy(Pinent *pp)
{
	Bprint(out, "%5s-%-5s %d/%d", pp->chname, pp->pname, pp->x, pp->y);
}

/*
 * print routing direction
 */
void
prdir(void)
{
	switch(dirn[0]){
	case 0:
		Bprint(out, "R");
		break;
	case 1:
		Bprint(out, "L");
		break;
	default:
		Bprint(out, " ");
	}
	switch(dirn[1]){
	case 0:
		Bprint(out, "U");
		break;
	case 1:
		Bprint(out, "D");
		break;
	default:
		Bprint(out, " ");
	}
}

/*
 * find origin point
 * find the board position
 * return zero or command code
 */
int
cornset(void)
{
	Pinent *pp;
	int i, c;

	if (corncnt == 0){
		fprint(2, "no board dimensions given\n");
		exits("cornset");
	}
	origin = 0;
	if(c = findz())
		return(c);
	print("check other corner positions\n");
	for(i = 0; i < corncnt; i++){
		if(i == origin)
			continue;
		pp = &corn[i];
		prpinxy(pp);
		Bprint(out, "\n");
		Bflush(out);
		movptr(pp->x, pp->y);
		if(c = rdcmd())	/* assign = */
			return c;
		absx = pp->x * TWOX;
		absy = pp->y * TWOY;
	}
	return 0;
}

/*
 * hunt for board origin
 */
int
findz(void)
{
	Pinent *pp;
	int c;

	calib = 0;
	pp = &corn[origin];
	putbin(DASH, DASH);
	Bprint(out, "move cursor to corner pin ");
	prpinxy(pp);
	Bprint(out, "\n");
	Bflush(out);
	while(c = rdcmd())	/* assign = */
		return(c);
	absx = pp->x * TWOX;
	absy = pp->y * TWOY;
	calib = 1;
	return 0;
}

/*
 * move pointer to origin
 */
void
home(void)
{
	Pinent *pp;

	pp = &corn[origin];
	putbin(DASH, DASH);
	movptr(pp->x, pp->y);
}


/*
 * wait for char from terminal
 * zero if space, newline or switch0 used
 * jog if direction char used
 */
#define SMALL	5
#define BIG	200
int
rdcmd(void)
{
	int c;
	int delx, dely;
	int n;

	for(;;){
		c = rdkey();
		delx = dely = 0;
		n=1;
		if(isdigit(c)){
			n=0;
			while(isdigit(c)){
				n = n*10 + (c-'0');
				c = rdkey();
			}
		}
		switch(c){
		case 'r':
			delx = SMALL;
			break;
		case 'l':
			delx = -(SMALL);
			break;
		case 'u':
			dely = SMALL;
			break;
		case 'd':
			dely = -(SMALL);
			break;
		case ' ':
		case '\r':
		case '\n':
		case STAT:
			return(0);
		case '?':
			if (bufset){
				prwir(1);
				Bflush(out);
			}else
				print("?");
			continue;
		default:
			return c;
		}
		wwmove(delx*n, dely*n);
	}
	return 0;	/* not reached */
}


/*
 * read and return addr of string
 */
char *
rdstr(void)
{
	static char cvec[32];
	char *cp;
	int c;

	cp = 0;
	while((c = rdkey()) > 0){
		if(c == STAT)
			return 0;
		print("%c", c);
		switch(c){
		case ' ':
		case '	':
		case '\r':
		case '\n':
			if(cp){
				*cp = 0;
				return cvec;
			}
			continue;
		default:
			if(cp == 0)
				cp = cvec;
			if(cp < &cvec[sizeof cvec-1])
				*cp++ = c;
		}
	}
	return 0;
}

/*
 * return wire number last wrapped for this
 * board.   Zero if board was finished or
 * no record of it having been started.
 */
int
findlast(void)
{
	int n;
	char buf[32];

	wfile = open(".lastwire", ORDWR);
	if(wfile<0){
		wfile = create(".lastwire", ORDWR, 0666);
		return 0;
	}
	n = read(wfile, buf, sizeof buf-1);
	if(n <= 0)
		return 0;
	buf[n] = 0;
	return strtol(buf, 0, 10);
}

/*
 * set .lastwire file to contain n
 */
void
lastw(int n)
{
	seek(wfile, 0, 0);
	fprint(wfile, "%d\n", n);
}
