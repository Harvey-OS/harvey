#include	"ddb.h"

void
main(int argc, char *argv[])
{
	int i, w;
	Rune r;
	ulong etype;

	tgsize = 0;
	for(i=1; i<argc; i++) {
		w = openfile(argv[i]);
		if(w < 0)
			exits("open");
		tgsize += seek(w, 0, 2);
		close(w);
	}
	games = malloc(tgsize);
	if(games == 0) {
		fprint(2, "out of memory\n");
		exits("mem");
	}
	gsize = 0;
	for(i=1; i<argc; i++) {
		readit(argv[i]);
		if(i <= 26) {
			gsizelist[i-1] = gsize;
			gsizelist[i] = 0;
		}
	}
	if(i <= 1) {
		readit("hist");
		gsizelist[0] = gsize;
		gsizelist[1] = 0;
	}

	/*
	 * count the number of games
	 */
	tgames = countgames() + 1;
	lastgame = 0;
	print("size = %.2fM\n", (gsize+tgames*sizeof(Str))/(double)(1<<20));
	print("total games = %ld\n", tgames-1);

	/*
	 * make a structure per game
	 */
	ngames = tgames;
	str = malloc(tgames*sizeof(Str));
	if(str == 0) {
		fprint(2, "out of memory\n");
		exits("mem");
	}
	buildgames();

	/*
	 * initialize event stuff
	 */
	binit(0, 0, "ddb");
	einit(Emouse|Ekeyboard);

	cinitf1 = 0;	/* minimal expansion */
	cinitf2 = 2;	/* figurine algebraic */
	chessinit('G', cinitf1, cinitf2);
	xorinit();

	sortby = Byfile;
	men3.item = men3l;
	men3.lasthit = sortby;

	gameno = 0;

	ereshaped(D->r);
	sortit(1);

	cmdi = 0;
	for(;;) {
		etype = eread(Emouse|Ekeyboard, &ev);
		if(etype & Emouse) {
			ogameno = gameno;
			str[gameno].position = curgame.position;
			if(button(1)) {
				if(ptinrect(ev.mouse.xy, d.vbar)) {
					i = scroll(&d.vbar, 1, SIZEV, funvu, 1);
					if(i < 0)
						setgame(ogameno);
				} else
				if(ptinrect(ev.mouse.xy, d.hbar)) {
					i = scroll(&d.hbar, 1, SIZEH, funhu, 0);
					if(i < 0)
						setgame(ogameno);
				} else
				if(ptinrect(ev.mouse.xy, d.board)) {
					moveselect(1);
				}
			} else
			if(button(2)) {
				if(ptinrect(ev.mouse.xy, d.vbar)) {
					ogameno = gameno;
					i = scroll(&d.vbar, 2, ngames, funv, 1);
					if(i < 0)
						setgame(ogameno);
				} else
				if(ptinrect(ev.mouse.xy, d.hbar)) {
					i = scroll(&d.hbar, 2, curgame.nmoves+1, funh, 0);
					if(i < 0)
						setgame(ogameno);
				}
			} else
			if(button(3)) {
				if(ptinrect(ev.mouse.xy, d.vbar)) {
					ogameno = gameno;
					i = scroll(&d.vbar, 3, SIZEV, funvd, 1);
					if(i < 0)
						setgame(ogameno);
				} else
				if(ptinrect(ev.mouse.xy, d.hbar)) {
					i = scroll(&d.hbar, 3, SIZEH, funhd, 0);
					if(i < 0)
						setgame(ogameno);
				} else {
					i = menuhit(3, &ev.mouse, &men3);
					if(i >= 0) {
						sortby = i;
						sortit(1);
					}
				}
			}
		}
		if(etype & Ekeyboard) {
			r = ev.kbdc;
			switch(r) {
			default:
				if(cmdi < sizeof(cmd))
					cmdi += runetochar(&cmd[cmdi], &r);
				break;

			case '\n':
				strcpy(chars, "");
				prline(8);
				str[gameno].position = curgame.position;
				strcpy(chars, cmd);
				prline(7);
				cmdi = 0;
				nodi = 0;
				cursorswitch(&thinking);
				yysyntax = 0;
				yyterminal = 0;
				yystring = 0;
				yyparse();
				freenodes();
				cursorswitch(0);
				cmdi = 0;
				continue;

			case 0x4:
				bitblt(D, d.screen.min, D, d.screen, 0);
				exits(0);

			case '\b':
				if(cmdi > 0)
					cmdi--;
				break;
			}
			cmd[cmdi] = 0;
			strcpy(chars, cmd);
			prline(7);
		}
	}
}

void
funv(long n)
{
	setgame(n);
}

void
funvu(long n)
{
	setgame(ogameno-n-1);
}

void
funvd(long n)
{
	setgame(ogameno+n+1);
}

void
funh(long n)
{
	setposn(n);
}

void
funhu(long n)
{
	setposn(str[gameno].position-n-1);
}

void
funhd(long n)
{
	setposn(str[gameno].position+n+1);
}

int
sortorder(Str *a, Str *b)
{
	char *p1, *p2;

	p1 = (char*)a->gp;
	p2 = (char*)b->gp;
	if(p1 > p2)
		return 1;
	return -1;
}

int
sortfile(Str *a, Str *b)
{
	int n;
	char *p1, *p2;

	p1 = (char*)a->gp;
	p2 = (char*)b->gp;
	n = numcmp(p1+Fileoffset, p2+Fileoffset);
	if(n)
		return n;
	n = strcmp(p1+a->whiteoffset, p2+b->whiteoffset);
	if(n)
		return n;
	n = strcmp(p1+a->blackoffset, p2+b->blackoffset);
	if(n)
		return n;
	n = strcmp(p1+a->eventoffset, p2+b->eventoffset);
	if(n)
		return n;
	n = strcmp(p1+a->dateoffset, p2+b->dateoffset);
	return n;
}

int
sortwhite(Str *a, Str *b)
{
	int n;
	char *p1, *p2;

	p1 = (char*)a->gp;
	p2 = (char*)b->gp;
	n = strcmp(p1+a->whiteoffset, p2+b->whiteoffset);
	if(n)
		return n;
	n = strcmp(p1+a->blackoffset, p2+b->blackoffset);
	if(n)
		return n;
	n = strcmp(p1+a->eventoffset, p2+b->eventoffset);
	if(n)
		return n;
	n = strcmp(p1+a->dateoffset, p2+b->dateoffset);
	return n;
}

int
sortblack(Str *a, Str *b)
{
	int n;
	char *p1, *p2;

	p1 = (char*)a->gp;
	p2 = (char*)b->gp;
	n = strcmp(p1+a->blackoffset, p2+b->blackoffset);
	if(n)
		return n;
	n = strcmp(p1+a->whiteoffset, p2+b->whiteoffset);
	if(n)
		return n;
	n = strcmp(p1+a->eventoffset, p2+b->eventoffset);
	if(n)
		return n;
	n = strcmp(p1+a->dateoffset, p2+b->dateoffset);
	return n;
}

int
sortopening(Str *a, Str *b)
{
	int n;
	char *p1, *p2;

	p1 = (char*)a->gp;
	p2 = (char*)b->gp;
	n = strcmp(p1+a->openingoffset, p2+b->openingoffset);
	if(n)
		return n;
	n = strcmp(p1+a->whiteoffset, p2+b->whiteoffset);
	if(n)
		return n;
	n = strcmp(p1+a->blackoffset, p2+b->blackoffset);
	if(n)
		return n;
	n = strcmp(p1+a->eventoffset, p2+b->eventoffset);
	if(n)
		return n;
	n = strcmp(p1+a->dateoffset, p2+b->dateoffset);
	return n;
}

int
sortevent(Str *a, Str *b)
{
	int n;
	char *p1, *p2;

	p1 = (char*)a->gp;
	p2 = (char*)b->gp;
	n = strcmp(p1+a->eventoffset, p2+b->eventoffset);
	if(n)
		return n;
	n = strcmp(p1+a->dateoffset, p2+b->dateoffset);
	if(n)
		return n;
	n = strcmp(p1+a->whiteoffset, p2+b->whiteoffset);
	if(n)
		return n;
	n = strcmp(p1+a->blackoffset, p2+b->blackoffset);
	return n;
}

int
sortdate(Str *a, Str *b)
{
	int n;
	char *p1, *p2;

	p1 = (char*)a->gp;
	p2 = (char*)b->gp;
	n = strcmp(p1+a->dateoffset, p2+b->dateoffset);
	if(n)
		return n;
	n = strcmp(p1+a->eventoffset, p2+b->eventoffset);
	if(n)
		return n;
	n = strcmp(p1+a->whiteoffset, p2+b->whiteoffset);
	if(n)
		return n;
	n = strcmp(p1+a->blackoffset, p2+b->blackoffset);
	return n;
}

void
ereshaped(Box r)
{
	int i, w, h, oside;

	/*
	 * build screen layout
	 */
	d.screen = inset(r, INSET);
	w = (d.screen.max.x - d.screen.min.x) -
		(BAR+INSET) - (0);
	h = (d.screen.max.y - d.screen.min.y) -
		(0) - (INSET+BAR+INSET+NLINE*CHT+0);
	i = h;
	if(i > w)
		i = w;

	/*
	 * set up d.board
	 *	i is width of board
	 */
	oside = d.side;
	d.side = i/8;
	i = d.side*8;

	d.board.min.x = d.screen.min.x + (BAR+INSET);
	d.board.max.x = d.board.min.x + i;
	d.board.min.y = d.screen.min.y;
	d.board.max.y = d.board.min.y + i;

	/*
	 * set up d.hbar
	 */
	d.hbar.min.x = d.board.min.x;
	d.hbar.max.x = d.board.max.x;
	d.hbar.min.y = d.board.max.y + INSET;
	d.hbar.max.y = d.hbar.min.y + BAR;

	/*
	 * set up d.header
	 */
	d.header.min.x = d.board.min.x;
	d.header.max.x = d.screen.max.x - INSET;
	d.header.min.y = d.hbar.max.y + INSET;
	d.header.max.y = d.header.min.y + NLINE*CHT;

	/*
	 * set up d.vbar
	 */
	d.vbar.min.x = d.screen.min.x;
	d.vbar.max.x = d.vbar.min.x + BAR;
	d.vbar.min.y = d.board.min.y;
	d.vbar.max.y = d.board.max.y;

	if(oside != d.side)
	for(i=0; piece[i]; i++) {
		if(bitpiece[i])
			bfree(bitpiece[i]);
		bitpiece[i] = draw(piece[i], d.side);
	}
	bitblt(D, d.screen.min, D, d.screen, 0);

	doutline(d.header);
	hdrsize = (d.header.max.x - d.header.min.x) / CHW - 2;

	memset(lastbd, -1, sizeof(lastbd));
	forcegame(gameno);
}

void
prline(int line)
{
	int l, y;

	y = d.header.min.y + line*CHT - CHT/2;
	l = strlen(chars);
	if(l < hdrsize)
		memset(chars+l, ' ', hdrsize-l);
	chars[hdrsize] = 0;
	string(D,
		Pt(d.header.min.x + 10, y),
			font, chars, S);
	if(line == 8)
		bflush();
}

void
forcegame(long gn)
{

	gameno = -1;
	setgame(gn);
}

void
forceposn(int pn)
{

	ginit(&initp);
	curgame.position = 0;
	setposn(pn);
}

void
setgame(long gn)
{

	if(gn >= ngames)
		gn = ngames-1;
	if(gn < 0)
		gn = 0;
	if(gn == gameno) {
		setposn(curgame.position);
		return;
	}
	gameno = gn;
	decode(&curgame, &str[gameno]);
	setscroll(1);

	forceposn(curgame.position);

	sprint(chars, "file:   %s %ld/%ld", curgame.file, gameno, ngames-1);
	prline(1);
	sprint(chars, "white:  %s", curgame.white);
	prline(2);
	sprint(chars, "black:  %s", curgame.black);
	prline(3);
	sprint(chars, "event:  %s %s", curgame.event, curgame.date);
	prline(4);
	sprint(chars, "result: %s %s", curgame.result, curgame.opening);
	prline(5);
}

void
setposn(int pn)
{
	int i, j, p;
	Point pt;
	Bitmap *pc;

	if(pn > curgame.nmoves)
		pn = curgame.nmoves;
	if(pn < 0)
		pn = 0;
	getposn(pn);
	for(i=0; i<8; i++) {
		for(j=0; j<8; j++) {
			p = board[i*8+j] & 017;
			if(p == lastbd[i*8+j])
				continue;
			lastbd[i*8+j] = p;
			pt = Pt(d.board.min.x+j*d.side, d.board.min.y+i*d.side);

			pc = bitpiece[12];		/* empty */
			if(p & 7) {			/* body */
				pc = bitpiece[(p&7) + 6-1];
				if(p & 010)
					pc = bitpiece[(p&7) + 0-1];
			}
			bitblt(D, pt, pc, pc->r, S);
			if((i^j) & 1) {			/* black square shading */
				pc = bitpiece[(p&7) + 14-1];
				bitblt(D, pt, pc, pc->r, DorS);
			}
		}
	}
	doutline(d.board);

out:
	setscroll(0);
}

void
getposn(int pn)
{
	int i;

	if(pn <= 0 || pn > curgame.nmoves) {
		ginit(&initp);
		curgame.position = 0;
		sprint(chars, "after:  init");
		goto out;
	}
	if(pn < curgame.position-pn) {
		ginit(&initp);
		curgame.position = 0;
	}
	pn--;
	for(i=curgame.position; i>pn; i--)
		rmove();
	for(; i<pn; i++)
		move(curgame.moves[i]);
	if(pn & 1)
		sprint(chars, "after:  %d. ... %G",
			(pn+1)/2, curgame.moves[pn]);
	else
		sprint(chars, "after:  %d. %G",
			(pn+2)/2, curgame.moves[pn]);
	move(curgame.moves[pn]);
	curgame.position = pn+1;

out:
	prline(6);
}

int
openfile(char *file)
{
	int fi, l;

	l = strlen(file);
	if(l >= 5 && strcmp(file+l-5, "m.out") == 0) {
		sprint(chars, "%s", file);
		fi = open(chars, OREAD);
		if(fi < 0) {
			sprint(chars, "/lib/chess/%s", file);
			fi = open(chars, OREAD);
		}
	} else {
		sprint(chars, "%s.m.out", file);
		fi = open(chars, OREAD);
		if(fi < 0) {
			sprint(chars, "/lib/chess/%s.m.out", file);
			fi = open(chars, OREAD);
		}
	}
	if(fi < 0) {
		print("cant open %s\n", file);
		return -1;
	}
	return fi;
}

void
readit(char *file)
{
	long n, m;
	int fi;

	fi = openfile(file);
	print("reading %s\n", chars);
	n = seek(fi, 0, 2);
	seek(fi, 0, 0);
	if(gsize+n > tgsize) {
		if(tgsize == 0)
			games = malloc(n);
		else
			games = realloc(games, gsize+n);
		if(games == 0) {
			fprint(2, "out of memory\n");
			exits("mem");
		}
		tgsize = gsize+n;
	}
	m = read(fi, &games[gsize/sizeof(short)], n);
	if(m != n) {
		fprint(2, "short read %ld %ld\n", m, n);
		exits("read");
	}
	close(fi);
	gsize += m;
}

/*
 * go through the games structure
 * count the games and byte-swap
 */
long
countgames(void)
{
	uchar *cp;
	ushort *sp, *ep;
	int n, endianok;
	short s;
	long ng;

	endianok = 0;
	s = (0x01<<8) | 0x02;
	cp = (uchar*)&s;
	if(cp[0] == 0x01 && cp[1] == 0x02)
		endianok = 1;

	sp = &games[0];
	ep = &games[gsize/sizeof(*games)];

	ng = 0;
	while(sp < ep) {
		ng++;
		cp = (uchar*)sp;
		n = (cp[0]<<8) | cp[1];
		*sp++ = n;
		sp += n;
		if(sp >= ep)
			break;
		cp = (uchar*)sp;
		n = (cp[0]<<8) | cp[1];
		*sp++ = n;

		if(endianok) {
			sp += n;
			continue;
		}

		while(n >= 5) {
			n -= 5;
			cp = (uchar*)sp;
			sp[0] = (cp[0]<<8) | cp[1];
			sp[1] = (cp[2]<<8) | cp[3];
			sp[2] = (cp[4]<<8) | cp[5];
			sp[3] = (cp[6]<<8) | cp[7];
			sp[4] = (cp[8]<<8) | cp[9];
			sp += 5;
		}

		while(n > 0) {
			n--;
			cp = (uchar*)sp;
			*sp++ = (cp[0]<<8) | cp[1];
		}
	}
	return ng;
}

char	fanalys[]	= "game0";
char	wanalys[]	= "Analysis,W";
char	banalys[]	= "Analysis,B";
char	eanalys[]	= "Analysis";
char	ranalys[]	= "none";
char	oanalys[]	= "A00/00 *";
char	danalys[]	= "1991";

void
buildgames(void)
{
	Str *s;
	char *cp;
	ushort *p, *ep, *gep;
	int n, ch;
	ulong b;

	s = str;
	ngames = 0;

	/*
	 * hand-craft game 0
	 */
	ngames++;
	cp = (char*)g0;

	strcpy(cp + Fileoffset, fanalys);
	s->whiteoffset = Fileoffset +
		strlen(cp + Fileoffset) + 1;
	strcpy(cp + s->whiteoffset, wanalys);
	s->blackoffset = s->whiteoffset +
		strlen(cp + s->whiteoffset) + 1;
	strcpy(cp + s->blackoffset, banalys);
	s->eventoffset = s->blackoffset +
		strlen(cp + s->blackoffset) + 1;
	strcpy(cp + s->eventoffset, eanalys);
	s->resultoffset = s->eventoffset +
		strlen(cp + s->eventoffset) + 1;
	strcpy(cp + s->resultoffset, ranalys);
	s->openingoffset = s->resultoffset +
		strlen(cp + s->resultoffset) + 1;
	strcpy(cp + s->openingoffset, oanalys);
	s->dateoffset = s->openingoffset +
		strlen(cp + s->openingoffset) + 1;
	strcpy(cp + s->dateoffset, danalys);

	n = s->dateoffset + strlen(cp + s->dateoffset) - 1;
	if(n & 1)
		n++;
	g0[0] = n/2;	/* shorts worth of strings */
	g0[n+1] = 0;	/* shorts worth of moves */

	s->gp = g0;
	s->mark = 0;
	s->position = 0;
	s++;

	ch = 0;
	b = genmark(ch+'a');
	gep = &games[gsizelist[ch]/sizeof(*games)];

	p = games;
	ep = &games[gsize/sizeof(*games)];
	while(p < ep) {
		if(p >= gep) {
			ch++;
			b = genmark(ch+'a');
			gep = &games[gsizelist[ch]/sizeof(*games)];
			if(gsizelist[ch] == 0)
				gep = ep;
		}
		ngames++;

		cp = (char*)p;

		s->gp = p;
		s->mark = b;
		s->whiteoffset = (strchr(cp + Fileoffset, 0) - cp) + 1;
		s->blackoffset = (strchr(cp + s->whiteoffset, 0) - cp) + 1;
		s->eventoffset = (strchr(cp + s->blackoffset, 0) - cp) + 1;
		s->resultoffset = (strchr(cp + s->eventoffset, 0) - cp) + 1;
		s->openingoffset = (strchr(cp + s->resultoffset, 0) - cp) + 1;
		s->dateoffset = (strchr(cp + s->openingoffset, 0) - cp) + 1;
		s->position = 0;

		s++;

		n = *p;
		p += n + 1;
		if(p >= ep)
			break;
		n = *p;
		p += n + 1;
	}
}

void
decode(Game *g, Str *s)
{
	ushort *p;
	char *cp;
	int n;

	p = s->gp;
	cp = (char*)p;

	g->file = cp + Fileoffset;
	g->white = cp + s->whiteoffset;
	g->black = cp + s->blackoffset;
	g->event = cp + s->eventoffset;
	g->result = cp + s->resultoffset;
	g->opening = cp + s->openingoffset;
	g->date = cp + s->dateoffset;
	g->position = s->position;

	n = *p;
	p += n + 1;
	n = *p;
	g->moves = (short*)(p + 1);
	g->nmoves = n;
}

void
doutline(Box x)
{
	segment(D, Pt(x.min.x, x.min.y),
		Pt(x.max.x, x.min.y), ~0, DorS);
	segment(D, Pt(x.max.x, x.min.y),
		Pt(x.max.x, x.max.y), ~0, DorS);
	segment(D, Pt(x.max.x, x.max.y),
		Pt(x.min.x, x.max.y), ~0, DorS);
	segment(D, Pt(x.min.x, x.max.y),
		Pt(x.min.x, x.min.y), ~0, DorS);
}

int
button(int n)
{

	return ev.mouse.buttons & (1<<(n-1));
}

void
setscroll(int vert)
{
	Box s;
	int xy1, xy2, xy3;
	long pos1, pos2, tot;

	if(vert)
		goto dovert;

dohoriz:
	s = d.hbar;
	bitblt(D, s.min, D, s, 0xf);
	pos1 = curgame.position;
	pos2 = pos1 + 1;
	tot = curgame.nmoves+1-1 + 1;
	if(tot <= 0)
		tot = 1;
	xy1 = d.hbar.max.x - d.hbar.min.x;

	xy2 = (pos1 * xy1) / tot;
	if(xy2 < 0)
		xy2 = 0;
	if(xy2 > xy1)
		xy2 = xy1;
	s.min.x = d.hbar.min.x + xy2;

	xy3 = (pos2 * xy1) / tot;
	if(xy3 <= xy2)
		xy3 = xy2+1;
	if(xy3 < 0)
		xy3 = s.min.x;
	if(xy3 > xy1)
		xy3 = xy1;
	s.max.x = d.hbar.min.x + xy3;
	goto out;

dovert:
	s = d.vbar;
	bitblt(D, s.min, D, s, 0xf);
	pos1 = gameno;
	pos2 = pos1 + SIZEV;
	tot = ngames-1 + SIZEV;
	if(tot <= 0)
		tot = 1;
	xy1 = d.vbar.max.y - d.vbar.min.y;

	xy2 = (pos1 * xy1) / tot;
	if(xy2 < 0)
		xy2 = 0;
	if(xy2 > xy1)
		xy2 = xy1;
	s.min.y = d.vbar.min.y + xy2;

	xy3 = (pos2 * xy1) / tot;
	if(xy3 <= xy2)
		xy3 = xy2+1;
	if(xy3 < 0)
		xy3 = 0;
	if(xy3 > xy1)
		xy3 = xy1;
	s.max.y = d.vbar.min.y + xy3;
	goto out;

out:
	bitblt(D, s.min, D, s, 0x0);
}

long
scroll(Box *bar, int but, long tot, void (*fun)(long), int vert)
{
	Box s;
	int in, x, y, oxy;
	long pos, opos;

	pos = -1;
	oxy = -1;
	opos = -1;
	s = inset(*bar, 1);
	if(vert)
		goto dovert;

dohoriz:
	y = bar->min.y + BAR/2;
	do {
		in = abs(ev.mouse.xy.y-y) <= BAR/2;
		while(in) {
			x = ev.mouse.xy.x;
			if(x < s.min.x)
				x = s.min.x;
			if(x >= s.max.x)
				x = s.max.x;
			if(!eqpt(ev.mouse.xy, Pt(x, y)))
				cursorset(Pt(x, y));
			if(x == oxy)
				break;
			oxy = x;
			pos = (tot * (x - bar->min.x)) / (bar->max.x - bar->min.x);
			if(pos == opos)
				break;
			opos = pos;
			(*fun)(pos);
			break;
		}
		eread(Emouse, &ev);
	} while(button(but));
	if(!in)
		pos = -1;
	return pos;

dovert:
	x = bar->min.x + BAR/2;
	do {
		in = abs(ev.mouse.xy.x-x) <= BAR/2;
		while(in) {
			y = ev.mouse.xy.y;
			if(y < s.min.y)
				y = s.min.y;
			if(y >= s.max.y)
				y = s.max.y;
			if(!eqpt(ev.mouse.xy, Pt(x, y)))
				cursorset(Pt(x, y));
			if(y == oxy)
				break;
			oxy = y;
			pos = (tot * (y - bar->min.y)) / (bar->max.y - bar->min.y);
			if(pos == opos)
				break;
			opos = pos;
			(*fun)(pos);
			break;
		}
		eread(Emouse, &ev);
	} while(button(but));
	if(!in)
		pos = -1;
	return pos;
}

void
sortit(int think)
{

	if(think)
		cursorswitch(&thinking);
	qsort(str+1, ngames-1, sizeof(Str),
		(sortby==Byorder)? sortorder:
		(sortby==Byfile)? sortfile:
		(sortby==Bywhite)? sortwhite:
		(sortby==Byblack)? sortblack:
		(sortby==Byevent)? sortevent:
		(sortby==Byopening)? sortopening:
		(sortby==Bydate)? sortdate:
			0);
	forcegame(1);
	if(think)
		cursorswitch(0);
}

int
numcmp(char *a, char *b)
{
	char c1, c2;
	int n;

	for(;;) {
		c1 = *a++;
		c2 = *b++;
		n = c1 - c2;
		if(n)
			break;
		if(c1 == 0)
			return 0;
	}
	for(;;) {
		if(!isdigit(c1)) {
			if(isdigit(c2))
				return -1;
			return n;
		}
		if(!isdigit(c2))
			return 1;
		c1 = *a++;
		c2 = *b++;
	}
	return 0;
}

void
light(int loc)
{
	Box r;

	r.min.x = d.board.min.x + (loc&7)*d.side;
	r.min.y = d.board.min.y + ((loc>>3)&7)*d.side;
	r.max.x = r.min.x + d.side - 1;
	r.max.y = r.min.y + d.side - 1;
	doutline(r);
	r = inset(r, 1);
	doutline(r);
	lastbd[loc] = -1;
}

int
mvcmp(short *a, short *b)
{
	int f1, f2;

	f1 = FROM(*a);
	f2 = FROM(*b);
	return (board[f1]&7) - (board[f2]&7);
}

void
moveselect(int but)
{
	int to, fr, m, i, j;
	short mv[MAXMG];

	i = (ev.mouse.xy.x - d.board.min.x) / d.side;
	j = (ev.mouse.xy.y - d.board.min.y) / d.side;
	to = j*8 + i;

	getmv(mv);
	j = board[to];
	if(j & 7)
		if(curgame.position & 1) {
			if(j & BLACK)
				goto searchfrom;
		} else
			if(!(j & BLACK))
				goto searchfrom;
	j = 0;
	for(i=0; m=mv[i]; i++)
		if(to == TO(m))
			mv[j++] = m;
	mv[j] = 0;
	qsort(mv, j, sizeof(short), mvcmp);
	if(m = mv[0]) {
		light(TO(m));
		light(FROM(m));
	}

	do {
		eread(Emouse, &ev);
	} while(button(but));

	i = (ev.mouse.xy.x - d.board.min.x) / d.side;
	j = (ev.mouse.xy.y - d.board.min.y) / d.side;
	fr = j*8 + i;
	if(fr == to)
		fr = FROM(mv[0]);

	m = 0;
	for(i=0; j=mv[i]; i++)
		if(fr == FROM(j))
			m = j;
	goto mkmove;

searchfrom:
	fr = to;
	j = 0;
	for(i=0; m=mv[i]; i++)
		if(fr == FROM(m))
			mv[j++] = m;
	mv[j] = 0;

	do {
		eread(Emouse, &ev);
	} while(button(but));

	i = (ev.mouse.xy.x - d.board.min.x) / d.side;
	j = (ev.mouse.xy.y - d.board.min.y) / d.side;
	to = j*8 + i;

	m = 0;
	for(i=0; j=mv[i]; i++)
		if(to == TO(j))
			m = j;
	goto mkmove;

mkmove:
	if(m == 0) {
		setgame(gameno);
		return;
	}
	if(gameno != 0) {
		memmove(&g0[g0[0]+2], curgame.moves, curgame.position*sizeof(ushort));
		str->position = curgame.position;
		lastgame = str[gameno].gp;
	}
	g0[g0[0]+str->position+2] = m;
	str->position++;
	g0[g0[0]+1] = str->position;
	forcegame(0);
}
