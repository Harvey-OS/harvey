#include <u.h>
#include <libc.h>
#include <ndraw.h>
#include <bio.h>
#include "cons.h"

char *term = "2621";

struct funckey fk[32];

void
emulate(void)
{
	char buf[BUFS+1];
	int n;
	int c;
	int standout = 0;
	int insmode = 0;

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
						n = number(buf, nil);
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
