/*
 *
 *	debugger
 *
 */

#include "defs.h"
#include "fns.h"

extern	char	lastc, peekc;

void
scanform(long icount, int prt, char *ifp, Map *map, int literal)
{
	char	*fp;
	char	c;
	int	fcount;
	ADDR	savdot;
	int firstpass;

	firstpass = 1;
	while (icount) {
		fp=ifp;
		savdot=dot;
		/*now loop over format*/
		while (*fp) {
			if (!isdigit(*fp))
				fcount = 1;
			else {
				fcount = 0;
				while (isdigit(c = *fp++)) {
					fcount *= 10;
					fcount += c-'0';
				}
				fp--;
			}
			if (*fp==0)
				break;
			fp=exform(fcount,prt,fp,map,literal,firstpass);
			firstpass = 0;
		}
		dotinc=dot-savdot;
		dot=savdot;
		if (--icount)
			dot=inkdot(dotinc);
	}
}

char *
exform(int fcount, int prt, char *ifp, Map *map, int literal, int firstpass)
{
	/* execute single format item `fcount' times
	 * sets `dotinc' and moves `dot'
	 * returns address of next format item
	 */
	vlong	v;
	long	w;
	ulong	savdot;
	char	*fp;
	char	c, modifier;
	int	i;
	ushort sh, *sp;
	uchar ch, *cp;
	Symbol s;
	char buf[512];

	fp = 0;
	while (fcount > 0) {
		fp = ifp;
		c = *fp;
		modifier = *fp++;
		if (firstpass) {
			firstpass = 0;
			if (!literal  && (c == 'i' || c == 'I' || c == 'M')
					&& (dot & (mach->pcquant-1))) {
				dprint("warning: instruction not aligned");
				printc('\n');
			}
			if (prt && modifier != 'a' && modifier != 'A') {
				symoff(buf, 512, dot, CANY);
				dprint("%s%c%16t", buf, map==symmap? '?':'/');
			}
		}
		if (charpos()==0 && modifier != 'a' && modifier != 'A')
			dprint("\t\t");
		switch(modifier) {

		case SPC:
		case TB:
			dotinc = 0;
			break;

		case 't':
		case 'T':
			dprint("%*t", fcount);
			dotinc = 0;
			return(fp);

		case 'a':
			symoff(buf, sizeof(buf), dot, CANY);
			dprint("%s%c%16t", buf, map==symmap? '?':'/');
			dotinc = 0;
			break;

		case 'A':
			dprint("%lux%10t", dot);
			dotinc = 0;
			break;

		case 'p':
			if (get4(map, dot, &w) < 0)
				error("%r");
			symoff(buf, sizeof(buf), w, CANY);
			dprint("%s%16t", buf);
			dotinc = mach->szaddr;
			break;

		case 'u':
		case 'd':
		case 'x':
		case 'o':
		case 'q':
			if (literal)
				sh = (ushort) dot;
			else if (get2(map, dot, &sh) < 0)
				error("%r");
			w = sh;
			dotinc = 2;
			if (c == 'u')
				dprint("%-8lud", w);
			else if (c == 'x')
				dprint("%-8lux", w);
			else if (c == 'd')
				dprint("%-8ld", w);
			else if (c == 'o')
				dprint("%-8#luo", w);
			else if (c == 'q')
				dprint("%-8#lo", w);
			break;

		case 'U':
		case 'D':
		case 'X':
		case 'O':
		case 'Q':
			if (literal)
				w = (long) dot;
			else if (get4(map, dot, &w) < 0)
				error("%r");
			dotinc = 4;
			if (c == 'U')
				dprint("%-16lud", w);
			else if (c == 'X')
				dprint("%-16lux", w);
			else if (c == 'D')
				dprint("%-16ld", w);
			else if (c == 'O')
				dprint("%-#16luo", w);
			else if (c == 'Q')
				dprint("%-#16lo", w);
			break;
		case 'Z':
		case 'V':
		case 'Y':
			if (literal)
				v = dot;
			else if (get8(map, dot, &v) < 0)
				error("%r");
			dotinc = 8;
			if (c == 'Y')
				dprint("%-20llux", v);
			else if (c == 'V')
				dprint("%-20lld", v);
			else if (c == 'Z')
				dprint("%-20llud", v);
			break;
		case 'B':
		case 'b':
		case 'c':
		case 'C':
			if (literal)
				ch = (uchar) dot;
			else if (get1(map, dot, &ch, 1)  < 0)
				error("%r");
			if (modifier == 'C')
				printesc(ch);
			else if (modifier == 'B' || modifier == 'b')
				dprint("%-8lux", (long) ch);
			else
				printc(ch);
			dotinc = 1;
			break;

		case 'r':
			if (literal)
				sh = (ushort) dot;
			else if (get2(map, dot, &sh) < 0)
				error("%r");
			dprint("%C", sh);
			dotinc = 2;
			break;

		case 'R':
			if (literal) {
				sp = (ushort*) &dot;
				dprint("%C%C", sp[0], sp[1]);
				endline();
				dotinc = 4;
				break;
			}
			savdot=dot;
			while ((i = get2(map, dot, &sh) > 0) && sh) {
				dot=inkdot(2);
				dprint("%C", sh);
				endline();
			}
			if (i < 0)
				error("%r");
			dotinc = dot-savdot+2;
			dot=savdot;
			break;

		case 's':
			if (literal) {
				cp = (uchar*) &dot;
				for (i = 0; i < 4; i++)
					buf[i] = cp[i];
				buf[i] = 0;
				dprint("%s", buf);
				endline();
				dotinc = 4;
				break;
			}
			savdot = dot;
			for(;;){
				i = 0;
				do{
					if (get1(map, dot, (uchar*)&buf[i], 1) < 0)
						error("%r");
					dot = inkdot(1);
					i++;
				}while(!fullrune(buf, i));
				if(buf[0] == 0)
					break;
				buf[i] = 0;
				dprint("%s", buf);
				endline();
			}
			dotinc = dot-savdot+1;
			dot = savdot;
			break;

		case 'S':
			if (literal) {
				cp = (uchar*) &dot;
				for (i = 0; i < 4; i++)
					printesc(cp[i]);
				endline();
				dotinc = 4;
				break;
			}
			savdot=dot;
			while ((i = get1(map, dot, &ch, 1) > 0) && ch) {
				dot=inkdot(1);
				printesc(ch);
				endline();
			}
			if (i < 0)
				error("%r");
			dotinc = dot-savdot+1;
			dot=savdot;
			break;


		case 'I':
		case 'i':
			dotinc = machdata->das(map, dot, modifier, buf, sizeof(buf));
			if (dotinc < 0)
				error("%r");
			dprint("%s\n", buf);
			break;

		case 'M':
			dotinc = machdata->hexinst(map, dot, buf, sizeof(buf));
			if (dotinc < 0)
				error("%r");
			dprint("%s", buf);
			if (*fp) {
				dotinc = 0;
				dprint("%48t");
			} else
				dprint("\n");
			break;

		case 'f':
			/* BUG: 'f' and 'F' assume szdouble is sizeof(vlong) in the literal case */
			if (literal) {
				v = machdata->swav((ulong)dot);
				memmove(buf, &v, mach->szfloat);
			}else if (get1(map, dot, (uchar*)buf, mach->szfloat) < 0)
				error("%r");
			machdata->sftos(buf, sizeof(buf), (void*) buf);
			dprint("%s\n", buf);
			dotinc = mach->szfloat;
			break;

		case 'F':
			/* BUG: 'f' and 'F' assume szdouble is sizeof(vlong) in the literal case */
			if (literal) {
				v = machdata->swav(dot);
				memmove(buf, &v, mach->szdouble);
			}else if (get1(map, dot, (uchar*)buf, mach->szdouble) < 0)
				error("%r");
			machdata->dftos(buf, sizeof(buf), (void*) buf);
			dprint("%s\n", buf);
			dotinc = mach->szdouble;
			break;

		case 'n':
		case 'N':
			printc('\n');
			dotinc=0;
			break;

		case '"':
			dotinc=0;
			while (*fp != '"' && *fp)
				printc(*fp++);
			if (*fp)
				fp++;
			break;

		case '^':
			dot=inkdot(-dotinc*fcount);
			return(fp);

		case '+':
			dot=inkdot((WORD)fcount);
			return(fp);

		case '-':
			dot=inkdot(-(WORD)fcount);
			return(fp);

		case 'z':
			if (findsym(dot, CTEXT, &s))
				dprint("%s() ", s.name);
			printsource(dot);
			printc(EOR);
			return fp;

		default:
			error("bad modifier");
		}
		if (map->seg[0].fd >= 0)
			dot=inkdot(dotinc);
		fcount--;
		endline();
	}

	return(fp);
}

void
printesc(int c)
{
	static char hex[] = "0123456789abcdef";

	if (c < SPC || c >= 0177)
		dprint("\\x%c%c", hex[(c&0xF0)>>4], hex[c&0xF]);
	else
		printc(c);
}

ADDR
inkdot(WORD incr)
{
	ADDR	newdot;

	newdot=dot+incr;
	if ((incr >= 0 && newdot < dot)
	||  (incr < 0 && newdot > dot))
		error("address wraparound");
	return(newdot);
}
