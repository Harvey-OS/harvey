/*
 * Known bugs:
 *
 * 1. We don't handle cursor movement characters inside escape sequences.
 * 	That is, ESC[2C moves two to the right, so ESC[2\bC is supposed to back
 *	up one and then move two to the right.
 *
 * 2. We don't handle tabstops past nelem(tabcol) columns.
 *
 * 3. We don't respect requests to do reverse video for the whole screen.
 *
 * 4. We ignore the ESC#n codes, so that we don't do double-width nor 
 * 	double-height lines, nor the ``fill the screen with E's'' confidence check.
 *
 * 5. Cursor key sequences aren't selected by keypad application mode.
 *
 * 6. "VT220" mode (-2) currently just switches the default cursor key
 *	functions (same as -a); it's still just a VT100 emulation.
 *
 * 7. VT52 mode and a few other rarely used features are not implemented.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <bio.h>
#include <ctype.h>
#include "cons.h"

int	wraparound = 1;
int	originrelative = 0;

int	tabcol[200];

struct funckey vt100fk[NKEYS] = {
	{ "up key",		"\033OA", },
	{ "down key",		"\033OB", },
	{ "left key",		"\033OD", },
	{ "right key",		"\033OC", },
};

struct funckey ansifk[NKEYS] = {
	{ "up key",		"\033[A", },
	{ "down key",		"\033[B", },
	{ "left key",		"\033[D", },
	{ "right key",		"\033[C", },
	{ "F1",			"\033OP", },
	{ "F2",			"\033OQ", },
	{ "F3",			"\033OR", },
	{ "F4",			"\033OS", },
	{ "F5",			"\033OT", },
	{ "F6",			"\033OU", },
	{ "F7",			"\033OV", },
	{ "F8",			"\033OW", },
	{ "F9",			"\033OX", },
	{ "F10",		"\033OY", },
	{ "F11",		"\033OZ", },
	{ "F12",		"\033O1", },
};

struct funckey vt220fk[NKEYS] = {
	{ "up key",		"\033[A", },
	{ "down key",		"\033[B", },
	{ "left key",		"\033[D", },
	{ "right key",		"\033[C", },
};

char gmap[256] = {
	['_']	' ',	/* blank */
	['\\']	'*',	/* diamond */
	['a']	'X',	/* checkerboard */
	['b']	'\t',	/* HT */
	['c']	'\x0C',	/* FF */
	['d']	'\r',	/* CR */
	['e']	'\n',	/* LF */
	['f']	'o',	/* degree */
	['g']	'+',	/* plus/minus */
	['h']	'\n',	/* NL, but close enough */
	['i']	'\v',	/* VT */
	['j']	'+',	/* lower right corner */
	['k']	'+',	/* upper right corner */
	['l']	'+',	/* upper left corner */
	['m']	'+',	/* lower left corner */
	['n']	'+',	/* crossing lines */
	['o']	'-',	/* horiz line - scan 1 */
	['p']	'-',	/* horiz line - scan 3 */
	['q']	'-',	/* horiz line - scan 5 */
	['r']	'-',	/* horiz line - scan 7 */
	['s']	'-',	/* horiz line - scan 9 */
	['t']	'+',	/* |-   */
	['u']	'+',	/* -| */
	['v']	'+',	/* upside down T */
	['w']	'+',	/* rightside up T */
	['x']	'|',	/* vertical bar */
	['y']	'<',	/* less/equal */
	['z']	'>',	/* gtr/equal */
	['{']	'p',	/* pi */
	['|']	'!',	/* not equal */
	['}']	'L',	/* pound symbol */
	['~']	'.',	/* centered dot: · */
};

static void setattr(int argc, int *argv);

void
emulate(void)
{
	char buf[BUFS+1];
	int n;
	int c;
	int operand[10];
	int noperand;
	int savex, savey, saveattribute, saveisgraphics;
	int isgraphics;
	int g0set, g1set;

	isgraphics = 0;
	g0set = 'B';	/* US ASCII */
	g1set = 'B';	/* US ASCII */
	savex = savey = 0;
	yscrmin = 0;
	yscrmax = ymax;
	saveattribute = 0;
	saveisgraphics = 0;
	/* set initial tab stops to DEC-standard 8-column spacing */
	for(c=0; (c+=8)<nelem(tabcol);)
		tabcol[c] = 1;

	for (;;) {
		if (y > ymax) {
			x = 0;
			newline();
		}
		buf[0] = get_next_char();
		buf[1] = '\0';
		switch(buf[0]) {

		case '\000':
		case '\001':
		case '\002':
		case '\003':
		case '\004':
		case '\005':
		case '\006':
			goto Default;

		case '\007':		/* bell */
			ringbell();
			break;

		case '\010':		/* backspace */
			if (x > 0)
				--x;
			break;

		case '\011':		/* tab to next tab stop; if none, to right margin */
			for(c=x+1; c<nelem(tabcol) && !tabcol[c]; c++)
				;
			if(c < nelem(tabcol))
				x = c;
			else
				x = xmax;
			break;

		case '\012':		/* linefeed */
		case '\013':
		case '\014':
			newline();
			attribute = 0;
			if (ttystate[cs->raw].nlcr)
				x = 0;
			break;

		case '\015':		/* carriage return */
			x = 0;
			attribute = 0;
			if (ttystate[cs->raw].crnl)
				newline();
			break;

		case '\016':	/* SO: invoke G1 char set */
			isgraphics = (isdigit(g1set));
			break;
		case '\017':	/* SI: invoke G0 char set */
			isgraphics = (isdigit(g0set));
			break;

		case '\020':	/* DLE */
		case '\021':	/* DC1 */
		case '\022':	/* XON */
		case '\023':	/* DC3 */
		case '\024':	/* XOFF */
		case '\025':	/* NAK */
		case '\026':	/* SYN */
		case '\027':	/* ETB */
		case '\030':	/* CAN: cancel escape sequence, display checkerboard (not implemented) */
		case '\031':	/* EM */
		case '\032':	/* SUB: same as CAN */
			goto Default;
;
		/* ESC, \033, is handled below */
		case '\034':	/* FS */
		case '\035':	/* GS */
		case '\036':	/* RS */
		case '\037':	/* US */
			break;
		case '\177':	/* delete: ignored */
			break;

		case '\033':
			switch(get_next_char()){
			/*
			 * 1 - graphic processor option on (no-op; not installed)
			 */
			case '1':
				break;

			/*
			 * 2 - graphic processor option off (no-op; not installed)
			 */
			case '2':
				break;

			/*
			 * 7 - save cursor position.
			 */
			case '7':
				savex = x;
				savey = y;
				saveattribute = attribute;
				saveisgraphics = isgraphics;
				break;

			/*
			 * 8 - restore cursor position.
			 */
			case '8':
				x = savex;
				y = savey;
				attribute = saveattribute;
				isgraphics = saveisgraphics;
				break;

			/*
			 * c - Reset terminal.
			 */
			case 'c':
				cursoron = 1;
				curson(0);
				ttystate[cs->raw].nlcr = 0;
				break;

			/*
			 * D - active position down a line, scroll if at bottom margin.
			 * (Original VT100 had a bug: tracked new-line/line-feed mode.)
			 */
			case 'D':
				if(++y > yscrmax) {
					y = yscrmax;
					scroll(yscrmin+1, yscrmax+1, yscrmin, yscrmax);
				}
				break;

			/*
			 * E - active position to start of next line, scroll if at bottom margin.
			 */
			case 'E':
				x = 0;
				if(++y > yscrmax) {
					y = yscrmax;
					scroll(yscrmin+1, yscrmax+1, yscrmin, yscrmax);
				}
				break;

			/*
			 * H - set tab stop at current column.
			 * (This is cursor home in VT52 mode (not implemented).)
			 */
			case 'H':
				if(x < nelem(tabcol))
					tabcol[x] = 1;
				break;

			/*
			 * M - active position up a line, scroll if at top margin..
			 */
			case 'M':
				if(--y < yscrmin) {
					y = yscrmin;
					scroll(yscrmin, yscrmax, yscrmin+1, yscrmin);
				}
				break;

			/*
			 * Z - identification.  the terminal
			 * emulator will return the response
			 * code for a generic VT100.
			 */
			case 'Z':
			Ident:
				sendnchars2(7, "\033[?1;2c");	/* VT100 with AVO option */
//				sendnchars2(5, "\033[?6c");	/* VT102 (insert/delete-char, etc.) */
				break;

			/*
			 * < - enter ANSI mode
			 */
			case '<':
				break;

			/*
			 * > - set numeric keypad mode on (not implemented)
			 */
			case '>':
				break;

			/*
			 * = - set numeric keypad mode off (not implemented)
			 */
			case '=':
				break;

			/*
			 * # - Takes a one-digit argument
			 */
			case '#':
				switch(get_next_char()){
				case '3':		/* Top half of double-height line */
				case '4':		/* Bottom half of double-height line */
				case '5':		/* Single-width single-height line */
				case '6':		/* Double-width line */
				case '7':		/* Screen print */
				case '8':		/* Fill screen with E's */
					break;
				}
				break;

			/*
			 * ( - switch G0 character set
			 */
			case '(':
				g0set = get_next_char();
				break;

			/*
			 * - switch G1 character set
			 */
			case ')':
				g1set = get_next_char();
				break;

			/*
			 * Received left bracket.
			 */
			case '[':
				/*
				 * A semi-colon or ? delimits arguments.
				 */
				memset(operand, 0, sizeof(operand));
				operand[0] = number(buf, nil);
				noperand = 1;
				while(buf[0] == ';' || buf[0] == '?'){
					if(noperand < nelem(operand)) {
						noperand++;
						operand[noperand-1] = number(buf, nil);
					} else
						number(buf, nil);
				}

				/*
				 * do escape2 stuff
				 */
				switch(buf[0]){
					/*
					 * c - same as ESC Z: what are you?
					 */
					case 'c':
						goto Ident;

					/*
					 * g - various tabstop manipulation
					 */
					case 'g':
						switch(operand[0]){
						case 0:	/* clear tab at current column */
							if(x < nelem(tabcol))
								tabcol[x] = 0;
							break;
						case 3:	/* clear all tabs */
							memset(tabcol, 0, sizeof tabcol);
							break;
						}
						break;

					/*
					 * l - clear various options.
					 */
					case 'l':
						if(noperand == 1){
							switch(operand[0]){	
							case 20:	/* set line feed mode */
								ttystate[cs->raw].nlcr = 0;
								break;
							case 30:	/* screen invisible (? not supported through VT220) */
								break;
							}
						}else while(--noperand > 0){
							switch(operand[noperand]){
							case 1:	/* set cursor keys to send ANSI functions: ESC [ A..D */
								break;
							case 2:	/* set VT52 mode (not implemented) */
								break;
							case 3:	/* set 80 columns */
								setdim(-1, 80);
								break;
							case 4:	/* set jump scrolling */
								break;
							case 5:	/* set normal video on screen */
								break;
							case 6:	/* set origin to absolute */
print("OL\n");
								originrelative = 0;
								x = y = 0;
								break;
							case 7:	/* reset auto-wrap mode */
								wraparound = 0;
								break;
							case 8:	/* reset auto-repeat mode */
								break;
							case 9:	/* reset interlacing mode */
								break;
							case 25:	/* text cursor off (VT220) */
								cursoron = 0;
								cursoff();
								break;
							}
						}
						break;

					/*
					 * h - set various options.
					 */
					case 'h':
//print("h%d:%d,%d\n", noperand, operand[0], operand[1]);
						if(noperand == 1){
							switch(operand[0]){	
							case 20:	/* set newline mode */
								ttystate[cs->raw].nlcr = 1;
								break;
							case 30:	/* screen visible (? not supported through VT220) */
								break;
							}
						}else while(--noperand > 0){
							switch(operand[noperand]){
							case 1:	/* set cursor keys to send application function: ESC O A..D */
								break;
							case 2:	/* set ANSI */
								break;
							case 3:	/* set 132 columns */
								setdim(-1, 132);
							break;
							case 4:	/* set smooth scrolling */
								break;
							case 5:	/* set screen to reverse video (not implemented) */
								break;
							case 6:	/* set origin to relative */
								originrelative = 1;
								x = 0;
								y = yscrmin;
								break;
							case 7:	/* set auto-wrap mode */
								wraparound = 1;
								break;
							case 8:	/* set auto-repeat mode */
								break;
							case 9:	/* set interlacing mode */
								break;
							case 25:	/* text cursor on (VT220) */
								cursoron = 1;
								curson(0);
								break;
							}
						}
						break;

					/*
					 * m - change character attributes.
					 */
					case 'm':
						setattr(noperand, operand);
						break;

					/*
					 * n - request various reports
					 */
					case 'n':
						switch(operand[0]){
						case 5:	/* status */
							sendnchars2(4, "\033[0n");	/* terminal ok */
							break;
						case 6:	/* cursor position */
							sendnchars2(sprint(buf, "\033[%d;%dR",
								originrelative ? y+1 - yscrmin : y+1, x+1), buf);
							break;
						}
						break;

					/*
					 * q - turn on list of LEDs; turn off others.
					 */
					case 'q':
//						print("LED\n");
						break;

					/*
					 * r - change scrolling region.  operand[0] is
					 * min scrolling region and operand[1] is max
					 * scrolling region.
					 */
					case 'r':
						yscrmin = 0;
						yscrmax = ymax;
						switch(noperand){
						case 2:
							yscrmax = operand[1]-1;
							if(yscrmax > ymax)
								yscrmax = ymax;
						case 1:
							yscrmin = operand[0]-1;
							if(yscrmin < 0)
								yscrmin = 0;
						}
						x = 0;
						y = yscrmin;
						break;

					/*
					 * x - report terminal parameters
					 */
					case 'x':
						sendnchars2(20, "\033[3;1;1;120;120;1;0x");
						break;

					/*
					 * y - invoke confidence test
					 */
					case 'y':
						break;

					/*
					 * A - cursor up.
					 */
					case 'A':
						if(operand[0] == 0)
							operand[0] = 1;
						y -= operand[0];
						if(y < yscrmin)
							y = yscrmin;
						olines -= operand[0];
						if(olines < 0)
							olines = 0;
						break;

					/*
					 * B - cursor down
					 */
					case 'B':
						if(operand[0] == 0)
							operand[0] = 1;
						y += operand[0];
						if(y > yscrmax)
							y=yscrmax;
						break;
					
					/*
					 * C - cursor right
					 */
					case 'C':
						if(operand[0] == 0)
							operand[0] = 1;
						x += operand[0];
						/*
						 * VT-100-UG says not to go past the
						 * right margin.
						 */
						if(x > xmax)
							x=xmax;
						break;

					/*
					 * D - cursor left
					 */
					case 'D':
						if(operand[0] == 0)
							operand[0] = 1;
						x -= operand[0];
						if(x < 0)
							x = 0;
						break;

					/*
					 * H and f - cursor motion.  operand[0] is row and
					 * operand[1] is column, origin 1.
					 */
					case 'H':
					case 'f':
						x = operand[1] - 1;
						if(x < 0)
							x = 0;
						if(x > xmax)
							x = xmax;
						y = operand[0] - 1;
						if(y < 0)
							y = 0;
						if(originrelative){
							y += yscrmin;
							if(y > yscrmax)
								y = ymax;
						}else{
							if(y > yscrmax)
								y = yscrmax;
						}
						break;

					/*
					 * J - clear some or all of the display.
					 */
					case 'J':
						switch (operand[0]) {
							/*
							 * operand 2:  whole screen.
							 */
							case 2:
								clear(Rpt(pt(0, 0), pt(xmax+1, ymax+1)));
								break;
							/*
							 * operand 1: start of screen to active position, inclusive.
							 */
							case 1:
								clear(Rpt(pt(0, 0), pt(xmax+1, y)));
								clear(Rpt(pt(0, y), pt(x+1, y+1)));
								break;
							/*
							 * Default:  active position to end of screen, inclusive.
							 */
							default:
								clear(Rpt(pt(x, y), pt(xmax+1, y+1)));
								clear(Rpt(pt(0, y+1), pt(xmax+1, ymax+1)));
								break;
						}
						break;

					/*
					 * K - clear some or all of the line.
					 */
					case 'K':
						switch (operand[0]) {
							/*
							 * operand 2: whole line.
							 */
							case 2:
								clear(Rpt(pt(0, y), pt(xmax+1, y+1)));
								break;
							/*
							 * operand 1: start of line to active position, inclusive.
							 */
							case 1:
								clear(Rpt(pt(0, y), pt(x+1, y+1)));
								break;
							/*
							 * Default: active position to end of line, inclusive.
							 */
							default:
								clear(Rpt(pt(x, y), pt(xmax+1, y+1)));
								break;
						}
						break;

					/*
					 * L - insert a line at cursor position (VT102 and later)
					 */
					case 'L':
						scroll(y, yscrmax, y+1, y);
						break;

					/*
					 * M - delete a line at cursor position (VT102 and later)
					 */
					case 'M':
						scroll(y+1, yscrmax+1, y, yscrmax);
						break;

					case '=':	/* ? not supported through VT220 */
						number(buf, nil);
						switch(buf[0]) {
						case 'h':
						case 'l':
							break;
						}
						break;

					/*
					 * Anything else we ignore for now...
					 */
					default:
						break;
				}

				break;

			/*
			 * Collapse multiple '\033' to one.
			 */
			case '\033':
				peekc = '\033';
				break;

			/*
			 * Ignore other commands.
			 */
			default:
				break;

			}
			break;

		default:		/* ordinary char */
Default:
			if(isgraphics && gmap[(uchar) buf[0]])
				buf[0] = gmap[(uchar) buf[0]];

			/* line wrap */
			if (x > xmax){
				if(wraparound){
					x = 0;
					newline();
				}else{
					continue;
				}
			}
			n = 1;
			c = 0;
			while (!cs->raw && host_avail() && x+n<=xmax && n<BUFS
			    && (c = get_next_char())>=' ' && c<'\177') {
				buf[n++] = c;
				c = 0;
			}
			buf[n] = 0;
//			clear(Rpt(pt(x,y), pt(x+n, y+1)));
			drawstring(pt(x, y), buf, attribute);
			x += n;
			peekc = c;
			break;
		}
	}
}

static void
setattr(int argc, int *argv)
{
	int i;

	for(i=0; i<argc; i++) {
		switch(argv[i]) {
		case 0:
			attribute = 0;
			break;
		case 1:
			attribute |= THighIntensity;
			break;		
		case 4:
			attribute |= TUnderline;
			break;		
		case 5:
			attribute |= TBlink;
			break;
		case 7:
			attribute |= TReverse;
			break;
		case 8:
			attribute |= TInvisible;
			break;
		case 22:
			attribute &= ~THighIntensity;
			break;		
		case 24:
			attribute &= ~TUnderline;
			break;		
		case 25:
			attribute &= ~TBlink;
			break;
		case 27:
			attribute &= ~TReverse;
			break;
		case 28:
			attribute &= ~TInvisible;
			break;
		case 30:
		case 31:
		case 32:
		case 33:
		case 34:
		case 35:
		case 36:
		case 37:
			frgcolor = argv[i]-30;
			break;
		case 40:
		case 41:
		case 42:
		case 43:
		case 44:
		case 45:
		case 46:
		case 47:
			bckcolor = argv[i]-40;
			break;
		}
	}
}
