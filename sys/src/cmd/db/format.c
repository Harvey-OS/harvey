/*
 *
 *	debugger
 *
 */

#include "defs.h"
#include "fns.h"

extern	char	lastc, peekc;

void
scanform(WORD icount, int prt, char *ifp, Map *map, int itype, int ptype)
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
		while (*fp && errflg==0) {
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
			fp=exform(fcount,prt,fp,map,itype,ptype,firstpass);
			firstpass = 0;
		}
		dotinc=dot-savdot;
		dot=savdot;
		if (errflg) {
			if (icount<0) {
				errflg=0;
				break;
			}
			else
				error(errflg);
		}
		if (--icount)
			dot=inkdot(dotinc);
		if (mkfault)
			error(0);
	}
}

char *
exform(int fcount, int prt, char *ifp, Map *map, int itype, int ptype, int firstpass)
{
	/* execute single format item `fcount' times
	 * sets `dotinc' and moves `dot'
	 * returns address of next format item
	 */
	WORD	w;
	ADDR	savdot;
	char	*fp;
	char	c, modifier;
	int	i;
	ushort sh;
	uchar ch;
	char	buf[32];
	Symbol s;

	fp = 0;
	while (fcount > 0) {
		fp = ifp;
		c = *fp;
		modifier = *fp++;
		if (firstpass) {
			firstpass = 0;
			if (itype != SEGNONE  && (c == 'i' || c == 'I' || c == 'M')
					&& (dot & (mach->pcquant-1))) {
				dprint("warning: instruction not aligned");
				printc(EOR);
			}
			if (prt)
				psymoff((WORD)dot,itype,map==symmap?"?%16t":"/%16t");
		}
		if (charpos()==0 && modifier!='a')
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
			psymoff((WORD)dot, ptype, map==symmap ?"?%16t":"/%16t");
			dotinc = 0;
			break;

		case 'p':
			if (get4(map, dot, itype, &w) == 0)
				return (fp);
			if (mkfault)
				return (0);
			psymoff(w, ptype, "%16t");
			dotinc = mach->szaddr;
			break;

		case 'u':
		case 'd':
		case 'x':
		case 'o':
		case 'q':
			if (get2(map, dot, itype, &sh) == 0)
				return (fp);
			w = sh;
			if (mkfault)
				return (0);
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
			if (get4(map, dot, itype, &w) == 0)
				return (fp);
			if (mkfault)
				return (0);
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
		case 'B':
		case 'b':
		case 'c':
		case 'C':
			if (get1(map, dot, itype, &ch, 1) == 0)
				return (fp);
			if (mkfault)
				return (0);
			if (modifier == 'C')
				printesc(ch);
			else if (modifier == 'B' || modifier == 'b')
				dprint("%-8lux", (long) ch);
			else
				printc(ch);
			dotinc = 1;
			break;

		case 'r':
			if (get2(map, dot, itype, &sh) == 0)
				return (fp);
			w = sh;
			if (mkfault)
				return (0);
			dprint("%C", w);
			dotinc = 2;
			break;

		case 'R':
			savdot=dot;
			dotinc=2;
			while (get2(map, dot,itype, &sh) && sh) {
				dot=inkdot(2);
				dprint("%C", sh);
				endline();
			}
			dotinc = dot-savdot+2;
			dot=savdot;
			break;

		case 's':
			savdot = dot;
			for(;;){
				i = 0;
				do{
					if (get1(map, dot, itype, (uchar *) &buf[i], 1) == 0)
						return fp;
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
			savdot=dot;
			dotinc=1;
			while (get1(map, dot,itype, &ch, 1) && ch) {
				dot=inkdot(1);
				printesc(ch);
				endline();
			}
			dotinc = dot-savdot+1;
			dot=savdot;
			break;

		case 'Y':
			get4(map, dot, itype, &w);
			printdate(w);
			dotinc = 4;
			break;

		case 'I':
		case 'i':
			machdata->printins(map, modifier, itype);
			printc(EOR);
			break;

		case 'M':
			machdata->printdas(map, itype);
			break;

		case 'f':
			if (get1(map, dot, itype, (uchar *)buf, mach->szfloat) == 0)
				return (fp);
			if (mkfault)
				return (0);
			machdata->sfpout(buf);
			dotinc = mach->szfloat;
			break;

		case 'F':
			if (get1(map, dot, itype, (uchar *) buf, mach->szdouble) == 0)
				return (fp);
			if (mkfault)
				return (0);
			machdata->dfpout(buf);
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
		if (itype != SEGNONE)
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
