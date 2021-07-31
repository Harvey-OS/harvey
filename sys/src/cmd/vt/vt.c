#include <u.h>
#include <libc.h>
#include <draw.h>
#include <bio.h>
#include <ctype.h>
#include "cons.h"

struct funckey vt100fk[NKEYS] = {
	{ "up key",		"\033OA", },
	{ "down key",		"\033OB", },
	{ "left key",		"\033OD", },
	{ "right key",		"\033OC", },
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

void
emulate(void)
{
	char buf[BUFS+1];
	int n;
	int c;
	int standout;
	int operand, prevOperand;
	int savex, savey;
	int isgraphics;
	int M, m;
	int g0set, g1set;

	standout = 0;
	isgraphics = 0;
	g0set = 'B';	/* US ASCII */
	g1set = 'B';	/* US ASCII */
	savex = savey = 0;

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
			break;

		case '\007':		/* bell */
			ringbell();
			break;

		case '\010':		/* backspace */
			if (x > 0)
				--x;
			break;

		case '\011':		/* tab modulo 8 */
			x = (x|7)+1;
			break;

		case '\012':		/* linefeed */
		case '\013':
		case '\014':
			newline();
			standout = 0;
			if (ttystate[cs->raw].nlcr)
				x = 0;
			break;

		case '\015':		/* carriage return */
			x = 0;
			standout = 0;
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
		case '\030':	/* CAN: cancel escape sequence */
		case '\031':	/* EM: cancel escape sequence, display checkerboard */
		case '\032':	/* SUB: same as CAN */
			break;
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
			 * 7 - save cursor position.
			 */
			case '7':
				savex = x;
				savey = y;
				break;

			/*
			 * 8 - restore cursor position.
			 */
			case '8':
				x = savex;
				y = savey;
				break;

			/*
			 * Received D.  Scroll forward.
			 */
			case 'D':
				M = yscrmax ? yscrmax : ymax;
				m = yscrmax ? yscrmin : 0;
				scroll(m+1, M+1, m, M);
				break;

			/*
			 * Received M.  Scroll reverse.
			 */
			case 'M':
				M = yscrmax ? yscrmax : ymax;
				m = yscrmax ? yscrmin : 0;
				scroll(m, M, m+1, m);
				break;

			/*
			 * Z - Identification.  The terminal
			 * emulator will return the response
			 * code for a generic VT100.
			 */
			case 'Z':
			Ident:
				sendnchars2(5, "\033[?6c");
				break;

			/*
			 * H - go home.
			 */
			case 'H':
				x = y = 0;
				break;

			/*
			 * > - set numeric keypad mode on
			 */
			case '>':
				break;

			/*
			 * = - set numeric keypad mode off
			 */
			case '=':
				break;

			/*
			 * # - Takes a one-digit argument that 
			 * we need to snarf away.
			 */
			case '#':
				get_next_char();
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
				 * A semi-colon or ? delimits arguments.  Only keep one
				 * previous argument (plus the current one) around.
				 */
				operand = number(buf);
				prevOperand = 0;
				while(buf[0] == ';' || buf[0] == '?'){
					prevOperand = operand;
					operand = number(buf);
				}

				/*
				 * do escape2 stuff
				 */
				switch(buf[0]){
					/*
					 * c - same as ESC Z: who are you?
					 */
					case 'c':
						goto Ident;

					/*
					 * l - set various options.
					 */
					case 'l':
						break;

					/*
					 * h - clear various options.
					 */
					case 'h':
						break;

					/*
					 * A - cursor up.
					 */
					case 'A':
						if(operand == 0)
							operand = 1;
						y -= operand;
						if(y < 0)
							y = 0;
						olines -= operand;
						if(olines < 0)
							olines = 0;
						break;

					/*
					 * B - cursor down
					 */
					case 'B':
						if(operand == 0)
							operand = 1;
						y += operand;
						while(y > ymax){
							scroll(1, ymax+1, 0, ymax);
							y--;
						}
						break;
					
					/*
					 * C - cursor right.
					 */
					case 'C':
						if(operand == 0)
							operand = 1;
						x += operand;
						break;

					/*
					 * D - cursor left (I think)
					 */
					case 'D':
						if(operand == 0)
							operand = 1;
						x -= operand;
						if(x < 0)
							x = 0;
						break;

					/*
					 * H - cursor motion.  operand is the column
					 * and prevOperand is the row, origin 1.
					 */
					case 'H':
						x = operand - 1;
						if(x < 0)
							x = 0;
						y = prevOperand - 1;
						if(y < 0)
							y = 0;
						break;

					/*
					 * J - clear in display.
					 */
					case 'J':
						switch (operand) {
							/*
							 * operand 2:  whole screen.
							 */
							case 2:
								clear(Rpt(pt(0, 0), pt(xmax+1, ymax+1)));
								break;

							/*
							 * Default:  clear to EOP.
							 */
							default:
								clear(Rpt(pt(0, y+1), pt(xmax+1, ymax+1)));
								break;
						}
						break;

					/*
					 * K - clear to EOL.
					 */
					case 'K':
						clear(Rpt(pt(x, y), pt(xmax+1, y+1)));
						break;

					/*
					 * L - insert a line at cursor position
					 */
					case 'L':
						M = yscrmax ? yscrmax : ymax;
						scroll(y, M, y+1, y);
						break;

					/*
					 * M - delete a line at the cursor position
					 */
					case 'M':
						M = yscrmax ? yscrmax : ymax;
						scroll(y+1, M+1, y, M);
						break;

					/*
					 * m - change character attributes.
					 */
					case 'm':
						standout = operand;
						break;

					/*
					 * r - change scrolling region.  prevOperand is
					 * min scrolling region and operand is max
					 * scrolling region.
					 */
					case 'r':
						yscrmin = prevOperand-1;
						yscrmax = operand-1;
						break;

					/*
					 * Anything else we ignore for now...
					 */
					default:
						break;
				}

				break;

			/*
			 * Ignore other commands.
			 */
			default:
				break;

			}
			break;

		default:		/* ordinary char */
			if(isgraphics && gmap[(uchar) buf[0]])
				buf[0] = gmap[(uchar) buf[0]];

			/* line wrap */
			if (x > xmax){
				x = 0;
				newline();
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
			drawstring(pt(x, y), buf, standout);
			x += n;
			peekc = c;
			break;
		}
	}
}
