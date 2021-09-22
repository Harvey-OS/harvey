#include	"all.h"

char*	htmlproto[] =
{
[0]	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\">\n"
	"<HTML>\n"
	"<HEAD>\n"
	"<TITLE>Board</TITLE>\n"
	"</HEAD>\n"
	"<BODY>\n",

[1]	"<TABLE border=1 align=left><TR><TD><TABLE cellspacing=0 cellpadding=0 border=0>\n",
[2]	"<TR>",
[3]	"<TD><IMG SRC=\"/who/ken/eggif/%s%d.gif\" width=%d height=%d>\n",
[4]	"</TR></TABLE></TD></TR></TABLE>",

[5]	"</BODY></HTML>\n\n",
};

void
flushhout(void)
{
	hflush(hout);
}

void
main(int argc, char *argv[])
{
	HConnect *c;
	char *s;

	atexit(flushhout);
	c = init(argc, argv);
	if(hparseheaders(c, 15*60*1000) < 0){
		logit(c, "chess header failed\n");
		exits("failed");
	}
	hout = &c->hout;
	if(c->req.vermaj) {
		hokheaders(c);
		hprint(hout, "Content-type: text/html\n");
		hprint(hout, "\r\n");
	}
	if(c->req.search) {
//		hprint(hout, " with search string %s", c->search);
	}

	s = setgame(c->req.uri);
	if(s != nil) {
		logit(c, "chesseg BAD %s", c->req.uri);
		hprint(hout, htmlproto[0]);
		hprint(hout, "<P>cannot set up board: %s\n", s);
		hprint(hout, htmlproto[5]);
		hflush(hout);
		exits(nil);
	}
	hprint(hout, htmlproto[0]);
	htmlbd();
	addptrs();
	hprint(hout, htmlproto[5]);
	hflush(hout);
	exits(nil);
}

char*
setgame(char *s)
{
	Posn p;
	int i, pc, lo, btm, wk, bk;
	static char diag[50];

	for(i=0; i<64; i++)
		p.bd[i] = 0;
	p.epp = 0;
	p.mvno = 0;
	p.mv[0] = 0;

	mateflag = 0;
	btm = 0;
	wk = 0;
	bk = 0;

	for(;;) {
		switch(*s) {
		default:
			sprint(diag, "unknown char: %c\n", *s);
			goto bad;

		case '\r':
		case '\n':
		case '/':
			s++;
			continue;

		case 'm':
			if(s[1] != '.' && s[1] != 'd' && s[1] != 0) {
				sprint(diag, "bad char after 'm': %c\n", *s);
				goto bad;
			}
			mateflag = 1;
			s++;
			continue;

		case '.':
		case 'd':
			if(s[1] != 0) {
				sprint(diag, "bad char after '%c': %c\n", s[-1], *s);
				goto bad;
			}
			btm = 1;
			s++;
			continue;

		case 0:
			goto done;

		case 'w':
		case 'W':
			pc = 0;
			break;
		case 'b':
		case 'B':
			pc = BLACK;
			break;
		}
		s++;

		switch(*s) {
		default:
			sprint(diag, "unknown char: %c\n", *s);
			goto bad;
		case 'k':
		case 'K':
			pc |= WKING;
			break;
		case 'q':
		case 'Q':
			pc |= WQUEEN;
			break;
		case 'r':
		case 'R':
			pc |= WROOK;
			break;
		case 'b':
		case 'B':
			pc |= WBISHOP;
			break;
		case 'n':
		case 'N':
			pc |= WKNIGHT;
			break;
		case 'p':
		case 'P':
			pc |= WPAWN;
			break;
		}
		s++;

		for(;;) {
			if(s[0] >= 'a' && s[0] <= 'h' && s[1] >= '1' && s[1] <= '8') {
				lo = (s[0]-'a') + ('8'-s[1])*8;
				goto set;
			}
			if(s[0] >= 'A' && s[0] <= 'H' && s[1] >= '1' && s[1] <= '8') {
				lo = (s[0]-'A') + ('8'-s[1])*8;
				goto set;
			}
			break;

		set:
			if(p.bd[lo]) {
				sprint(diag, "two pieces on one square %d\n", lo);
				goto bad;
			}
			if(pc == WKING)
				wk = 1;
			if(pc == BKING)
				bk = 1;
			p.bd[lo] = pc;
			s += 2;
		}
	}

done:
	if(wk == 0 || bk == 0) {
		sprint(diag, "missing king\n");
		goto bad;
	}
	ginit(&p);
	if(btm)
		move(Nomove);
	return nil;

bad:
	return diag;
}

void
wsearch(void)
{
	short mv[MAXMG];
	int j, m;

	lmp = mv;
	wgen();
	*lmp = 0;
	for(j=0; m=mv[j]; j++) {
		wmove(m);
		if(battack(wkpos))
			USED(j);
		wremove();
	}
}

void
bsearch(void)
{
	short mv[MAXMG];
	int j, m;

	lmp = mv;
	bgen();
	*lmp = 0;
	for(j=0; m=mv[j]; j++) {
		bmove(m);
		if(wattack(bkpos))
			USED(j);
		bremove();
	}
}

void
htmlbd(void)
{
	int i, j, p;
	char piece[20];

	hprint(hout, htmlproto[1]);
	for(i=0; i<8; i++) {
		hprint(hout, htmlproto[2]);
		for(j=0; j<8; j++) {
			p = board[i*8 + j] & 017;
			piece[0] = "ewwwwwwgebbbbbbg"[p];
			piece[1] = "epnbrqkgepnbrqkg"[p];
			piece[2] = "01"[(i^j)&1];
			piece[3] = 0;
			hprint(hout, htmlproto[3], piece, Side, Side, Side);
		}
	}
	hprint(hout, htmlproto[4]);
}
