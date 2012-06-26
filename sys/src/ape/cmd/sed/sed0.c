#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "sed.h"

struct label	*labtab = ltab;
char	CGMES[]	= "sed: Command garbled: %s\n";
char	TMMES[]	= "sed: Too much text: %s\n";
char	LTL[]	= "sed: Label too long: %s\n";
char	AD0MES[]	= "sed: No addresses allowed: %s\n";
char	AD1MES[]	= "sed: Only one address allowed: %s\n";
uchar	bittab[]  = {
		1,
		2,
		4,
		8,
		16,
		32,
		64,
		128
	};

void
main(int argc, char **argv)
{

	eargc = argc;
	eargv = (uchar**)argv;

	badp = &bad;
	aptr = abuf;
	hspend = holdsp;
	lab = labtab + 1;	/* 0 reserved for end-pointer */
	rep = ptrspace;
	rep->r1.ad1 = respace;
	lbend = &linebuf[LBSIZE];
	hend = &holdsp[LBSIZE];
	lcomend = &genbuf[64];
	ptrend = &ptrspace[PTRSIZE];
	reend = &respace[RESIZE];
	labend = &labtab[LABSIZE];
	lnum = 0;
	pending = 0;
	depth = 0;
	spend = linebuf;
	hspend = holdsp;
	fcode[0] = stdout;
	nfiles = 1;
	lastre = NULL;

	if(eargc == 1)
		exit(0);


	while (--eargc > 0 && (++eargv)[0][0] == '-')
		switch (eargv[0][1]) {

		case 'n':
			nflag++;
			continue;

		case 'f':
			if(eargc-- <= 0)	exit(2);

			if((fin = fopen((char*)(*++eargv), "r")) == NULL) {
				fprintf(stderr, "sed: Cannot open pattern-file: %s\n", *eargv);
				exit(2);
			}

			fcomp();
			fclose(fin);
			continue;

		case 'e':
			eflag++;
			fcomp();
			eflag = 0;
			continue;

		case 'g':
			gflag++;
			continue;

		default:
			fprintf(stderr, "sed: Unknown flag: %c\n", eargv[0][1]);
			continue;
		}


	if(compfl == 0) {
		eargv--;
		eargc++;
		eflag++;
		fcomp();
		eargv++;
		eargc--;
		eflag = 0;
	}

	if(depth) {
		fprintf(stderr, "sed: Too many {'s\n");
		exit(2);
	}

	labtab->address = rep;

	dechain();

/*	abort();	/*DEBUG*/

	if(eargc <= 0)
		execute((uchar *)NULL);
	else while(--eargc >= 0) {
		execute(*eargv++);
	}
	fclose(stdout);
	exit(0);
}
void
fcomp(void)
{

	uchar	*p, *op, *tp;
    uchar *address(uchar*);
	union reptr	*pt, *pt1;
	int	i;
	struct label	*lpt;

	compfl = 1;
	op = lastre;

	if(rline(linebuf) < 0) {
		lastre = op;
		return;
	}
	if(*linebuf == '#') {
		if(linebuf[1] == 'n')
			nflag = 1;
	}
	else {
		cp = linebuf;
		goto comploop;
	}

	for(;;) {
		if(rline(linebuf) < 0)	break;

		cp = linebuf;

comploop:
/*	fprintf(stdout, "cp: %s\n", cp);	/*DEBUG*/
		while(*cp == ' ' || *cp == '\t')	cp++;
		if(*cp == '\0' || *cp == '#')		continue;
		if(*cp == ';') {
			cp++;
			goto comploop;
		}

		p = address(rep->r1.ad1);
		if(p == badp) {
			fprintf(stderr, CGMES, linebuf);
			exit(2);
		}

		if(p == 0) {
			p = rep->r1.ad1;
			rep->r1.ad1 = 0;
		} else {
			if(p == rep->r1.ad1) {
				if(op)
					rep->r1.ad1 = op;
				else {
					fprintf(stderr, "sed: First RE may not be null\n");
					exit(2);
				}
			}
			if(*rep->r1.ad1 != CLNUM && *rep->r1.ad1 != CEND)
				op = rep->r1.ad1;
			if(*cp == ',' || *cp == ';') {
				cp++;
				if((rep->r1.ad2 = p) > reend) {
					fprintf(stderr, TMMES, linebuf);
					exit(2);
				}
				p = address(rep->r1.ad2);
				if(p == badp || p == 0) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if(p == rep->r1.ad2)
					rep->r1.ad2 = op;
				else{
				if(*rep->r1.ad2 != CLNUM && *rep->r1.ad2 != CEND)
					op = rep->r1.ad2;
				}

			} else
				rep->r1.ad2 = 0;
		}

		if(p > reend) {
			fprintf(stderr, "sed: Too much text: %s\n", linebuf);
			exit(2);
		}

		while(*cp == ' ' || *cp == '\t')	cp++;

swit:
		switch(*cp++) {

			default:
/*fprintf(stderr, "cp = %d; *cp = %o\n", cp - linebuf, *cp);*/
				fprintf(stderr, "sed: Unrecognized command: %s\n", linebuf);
				exit(2);

			case '!':
				rep->r1.negfl = 1;
				goto swit;

			case '{':
				rep->r1.command = BCOM;
				rep->r1.negfl = !(rep->r1.negfl);
				cmpend[depth++] = &rep->r2.lb1;
				if(++rep >= ptrend) {
					fprintf(stderr, "sed: Too many commands: %s\n", linebuf);
					exit(2);
				}
				rep->r1.ad1 = p;
				if(*cp == '\0')	continue;

				goto comploop;

			case '}':
				if(rep->r1.ad1) {
					fprintf(stderr, AD0MES, linebuf);
					exit(2);
				}

				if(--depth < 0) {
					fprintf(stderr, "sed: Too many }'s\n");
					exit(2);
				}
				*cmpend[depth] = rep;

				rep->r1.ad1 = p;
				if(*cp == 0)	continue;
				goto comploop;

			case '=':
				rep->r1.command = EQCOM;
				if(rep->r1.ad2) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				break;

			case ':':
				if(rep->r1.ad1) {
					fprintf(stderr, AD0MES, linebuf);
					exit(2);
				}

				while(*cp++ == ' ');
				cp--;


				tp = lab->asc;
				while((*tp = *cp++) && *tp != ';')
					if(++tp >= &(lab->asc[8])) {
						fprintf(stderr, LTL, linebuf);
						exit(2);
					}
				*tp = '\0';
				if(*lab->asc == 0) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}

				if(lpt = search(lab)) {
					if(lpt->address) {
						fprintf(stderr, "sed: Duplicate labels: %s\n", linebuf);
						exit(2);
					}
				} else {
					lab->chain = 0;
					lpt = lab;
					if(++lab >= labend) {
						fprintf(stderr, "sed: Too many labels: %s\n", linebuf);
						exit(2);
					}
				}
				lpt->address = rep;
				rep->r1.ad1 = p;

				continue;

			case 'a':
				rep->r1.command = ACOM;
				if(rep->r1.ad2) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				if(*cp == '\\')	cp++;
				if(*cp++ != '\n') {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->r1.re1 = p;
				p = text(rep->r1.re1);
				break;
			case 'c':
				rep->r1.command = CCOM;
				if(*cp == '\\')	cp++;
				if(*cp++ != ('\n')) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->r1.re1 = p;
				p = text(rep->r1.re1);
				break;
			case 'i':
				rep->r1.command = ICOM;
				if(rep->r1.ad2) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				if(*cp == '\\')	cp++;
				if(*cp++ != ('\n')) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->r1.re1 = p;
				p = text(rep->r1.re1);
				break;

			case 'g':
				rep->r1.command = GCOM;
				break;

			case 'G':
				rep->r1.command = CGCOM;
				break;

			case 'h':
				rep->r1.command = HCOM;
				break;

			case 'H':
				rep->r1.command = CHCOM;
				break;

			case 't':
				rep->r1.command = TCOM;
				goto jtcommon;

			case 'b':
				rep->r1.command = BCOM;
jtcommon:
				while(*cp++ == ' ');
				cp--;

				if(*cp == '\0') {
					if(pt = labtab->chain) {
						while(pt1 = pt->r2.lb1)
							pt = pt1;
						pt->r2.lb1 = rep;
					} else
						labtab->chain = rep;
					break;
				}
				tp = lab->asc;
				while((*tp = *cp++) && *tp != ';')
					if(++tp >= &(lab->asc[8])) {
						fprintf(stderr, LTL, linebuf);
						exit(2);
					}
				cp--;
				*tp = '\0';
				if(*lab->asc == 0) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}

				if(lpt = search(lab)) {
					if(lpt->address) {
						rep->r2.lb1 = lpt->address;
					} else {
						pt = lpt->chain;
						while(pt1 = pt->r2.lb1)
							pt = pt1;
						pt->r2.lb1 = rep;
					}
				} else {
					lab->chain = rep;
					lab->address = 0;
					if(++lab >= labend) {
						fprintf(stderr, "sed: Too many labels: %s\n", linebuf);
						exit(2);
					}
				}
				break;

			case 'n':
				rep->r1.command = NCOM;
				break;

			case 'N':
				rep->r1.command = CNCOM;
				break;

			case 'p':
				rep->r1.command = PCOM;
				break;

			case 'P':
				rep->r1.command = CPCOM;
				break;

			case 'r':
				rep->r1.command = RCOM;
				if(rep->r1.ad2) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				if(*cp++ != ' ') {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->r1.re1 = p;
				p = text(rep->r1.re1);
				break;

			case 'd':
				rep->r1.command = DCOM;
				break;

			case 'D':
				rep->r1.command = CDCOM;
				rep->r2.lb1 = ptrspace;
				break;

			case 'q':
				rep->r1.command = QCOM;
				if(rep->r1.ad2) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				break;

			case 'l':
				rep->r1.command = LCOM;
				break;

			case 's':
				rep->r1.command = SCOM;
				seof = *cp++;
				rep->r1.re1 = p;
				p = compile(rep->r1.re1);
				if(p == badp) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if(p == rep->r1.re1) {
					if(op == NULL) {
						fprintf(stderr, "sed: First RE may not be null.\n");
						exit(2);
					}
					rep->r1.re1 = op;
				} else {
					op = rep->r1.re1;
				}

				if((rep->r1.rhs = p) > reend) {
					fprintf(stderr, TMMES, linebuf);
					exit(2);
				}

				if((p = compsub(rep->r1.rhs)) == badp) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if(*cp == 'g') {
					cp++;
					rep->r1.gfl++;
				} else if(gflag)
					rep->r1.gfl++;

				if(*cp == 'p') {
					cp++;
					rep->r1.pfl = 1;
				}

				if(*cp == 'P') {
					cp++;
					rep->r1.pfl = 2;
				}

				if(*cp == 'w') {
					cp++;
					if(*cp++ !=  ' ') {
						fprintf(stderr, CGMES, linebuf);
						exit(2);
					}
					if(nfiles >= MAXFILES) {
						fprintf(stderr, "sed: Too many files in w commands 1 \n");
						exit(2);
					}

					text((uchar*)fname[nfiles]);
					for(i = nfiles - 1; i >= 0; i--)
						if(cmp((uchar*)fname[nfiles],(uchar*)fname[i]) == 0) {
							rep->r1.fcode = fcode[i];
							goto done;
						}
					if((rep->r1.fcode = fopen(fname[nfiles], "w")) == NULL) {
						fprintf(stderr, "sed: Cannot open %s\n", fname[nfiles]);
						exit(2);
					}
					fcode[nfiles++] = rep->r1.fcode;
				}
				break;

			case 'w':
				rep->r1.command = WCOM;
				if(*cp++ != ' ') {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if(nfiles >= MAXFILES){
					fprintf(stderr, "sed: Too many files in w commands 2 \n");
					fprintf(stderr, "nfiles = %d; MAXF = %d\n", nfiles, MAXFILES);
					exit(2);
				}

				text((uchar*)fname[nfiles]);
				for(i = nfiles - 1; i >= 0; i--)
					if(cmp((uchar*)fname[nfiles], (uchar*)fname[i]) == 0) {
						rep->r1.fcode = fcode[i];
						goto done;
					}

				if((rep->r1.fcode = fopen(fname[nfiles], "w")) == NULL) {
					fprintf(stderr, "sed: Cannot create %s\n", fname[nfiles]);
					exit(2);
				}
				fcode[nfiles++] = rep->r1.fcode;
				break;

			case 'x':
				rep->r1.command = XCOM;
				break;

			case 'y':
				rep->r1.command = YCOM;
				seof = *cp++;
				rep->r1.re1 = p;
				p = ycomp(rep->r1.re1);
				if(p == badp) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if(p > reend) {
					fprintf(stderr, TMMES, linebuf);
					exit(2);
				}
				break;

		}
done:
		if(++rep >= ptrend) {
			fprintf(stderr, "sed: Too many commands, last: %s\n", linebuf);
			exit(2);
		}

		rep->r1.ad1 = p;

		if(*cp++ != '\0') {
			if(cp[-1] == ';')
				goto comploop;
			fprintf(stderr, CGMES, linebuf);
			exit(2);
		}

	}
}

uchar	*
compsub(uchar *rhsbuf)
{
	uchar	*p, *q, *r;
	p = rhsbuf;
	q = cp;
	for(;;) {
		if((*p = *q++) == '\\') {
			*++p = *q++;
			if(*p >= '1' && *p <= '9' && *p > numbra + '0')
				return(badp);
			if(*p == 'n')
				*--p = '\n';
		} else if(*p == seof) {
			*p++ = '\0';
			cp = q;
			return(p);
		}
		if(*p++ == '\0') {
			return(badp);
		}

	}
}

uchar *
compile(uchar *expbuf)
{
	int c;
	uchar *ep, *sp;
	uchar	neg;
	uchar *lastep, *cstart;
	int cclcnt;
	int	closed;
	uchar	bracket[NBRA], *bracketp;

	if(*cp == seof) {
		cp++;
		return(expbuf);
	}

	ep = expbuf;
	lastep = 0;
	bracketp = bracket;
	closed = numbra = 0;
	sp = cp;
	if (*sp == '^') {
		*ep++ = 1;
		sp++;
	} else {
		*ep++ = 0;
	}
	for (;;) {
		if (ep >= reend) {
			cp = sp;
			return(badp);
		}
		if((c = *sp++) == seof) {
			if(bracketp != bracket) {
				cp = sp;
				return(badp);
			}
			cp = sp;
			*ep++ = CEOF;
			return(ep);
		}
		if(c != '*')
			lastep = ep;
		switch (c) {

		case '\\':
			if((c = *sp++) == '(') {
				if(numbra >= NBRA) {
					cp = sp;
					return(badp);
				}
				*bracketp++ = numbra;
				*ep++ = CBRA;
				*ep++ = numbra++;
				continue;
			}
			if(c == ')') {
				if(bracketp <= bracket) {
					cp = sp;
					return(badp);
				}
				*ep++ = CKET;
				*ep++ = *--bracketp;
				closed++;
				continue;
			}

			if(c >= '1' && c <= '9') {
				if((c -= '1') >= closed)
					return(badp);
	
				*ep++ = CBACK;
				*ep++ = c;
				continue;
			}
			if(c == '\n') {
				cp = sp;
				return(badp);
			}
			if(c == 'n') {
				c = '\n';
			}
			goto defchar;

		case '\0':
		case '\n':
			cp = sp;
			return(badp);

		case '.':
			*ep++ = CDOT;
			continue;

		case '*':
			if (lastep == 0)
				goto defchar;
			if(*lastep == CKET) {
				cp = sp;
				return(badp);
			}
			*lastep |= STAR;
			continue;

		case '$':
			if (*sp != seof)
				goto defchar;
			*ep++ = CDOL;
			continue;

		case '[':
			if(&ep[33] >= reend) {
				fprintf(stderr, "sed: RE too long: %s\n", linebuf);
				exit(2);
			}

			*ep++ = CCL;

			neg = 0;
			if((c = *sp++) == '^') {
				neg = 1;
				c = *sp++;
			}

			cstart = sp;
			do {
				if(c == '\0') {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if (c=='-' && sp>cstart && *sp!=']') {
					for (c = sp[-2]; c<*sp; c++)
						ep[c>>3] |= bittab[c&07];
				}
				if(c == '\\') {
					switch(c = *sp++) {
						case 'n':
							c = '\n';
							break;
					}
				}

				ep[c >> 3] |= bittab[c & 07];
			} while((c = *sp++) != ']');

			if(neg)
				for(cclcnt = 0; cclcnt < 32; cclcnt++)
					ep[cclcnt] ^= -1;
			ep[0] &= 0376;

			ep += 32;

			continue;

		defchar:
		default:
			*ep++ = CCHR;
			*ep++ = c;
		}
	}
}
int
rline(uchar *lbuf)
{
	uchar	*p, *q;
	int	t;
	static uchar	*saveq;

	p = lbuf - 1;

	if(eflag) {
		if(eflag > 0) {
			eflag = -1;
			if(eargc-- <= 0)
				exit(2);
			q = *++eargv;
			while(*++p = *q++) {
				if(*p == '\\') {
					if((*++p = *q++) == '\0') {
						saveq = 0;
						return(-1);
					} else
						continue;
				}
				if(*p == '\n') {
					*p = '\0';
					saveq = q;
					return(1);
				}
			}
			saveq = 0;
			return(1);
		}
		if((q = saveq) == 0)	return(-1);

		while(*++p = *q++) {
			if(*p == '\\') {
				if((*++p = *q++) == '0') {
					saveq = 0;
					return(-1);
				} else
					continue;
			}
			if(*p == '\n') {
				*p = '\0';
				saveq = q;
				return(1);
			}
		}
		saveq = 0;
		return(1);
	}

	while((t = getc(fin)) != EOF) {
		*++p = t;
		if(*p == '\\') {
			t = getc(fin);
			*++p = t;
		}
		else if(*p == '\n') {
			*p = '\0';
			return(1);
		}
	}
	*++p = '\0';
	return(-1);
}

uchar *
address(uchar *expbuf)
{
	uchar	*rcp;
	long	lno;

	if(*cp == '$') {
		cp++;
		*expbuf++ = CEND;
		*expbuf++ = CEOF;
		return(expbuf);
	}

	if(*cp == '/') {
		seof = '/';
		cp++;
		return(compile(expbuf));
	}

	rcp = cp;
	lno = 0;

	while(*rcp >= '0' && *rcp <= '9')
		lno = lno*10 + *rcp++ - '0';

	if(rcp > cp) {
		if(!lno){
			fprintf(stderr, "sed: line number 0 is illegal\n");
			exit(2);
		}
		*expbuf++ = CLNUM;
		*expbuf++ = lno;
		*expbuf++ = lno >> 8;
		*expbuf++ = lno >> 16;
		*expbuf++ = lno >> 24;
		*expbuf++ = CEOF;
		cp = rcp;
		return(expbuf);
	}
	return(0);
}
int
cmp(uchar *a, uchar *b)
{
	uchar	*ra, *rb;

	ra = a - 1;
	rb = b - 1;

	while(*++ra == *++rb)
		if(*ra == '\0')	return(0);
	return(1);
}

uchar *
text(uchar *textbuf)
{
	uchar	*p, *q;

	p = textbuf;
	q = cp;
	while(*q == '\t' || *q == ' ')	q++;
	for(;;) {

		if((*p = *q++) == '\\')
			*p = *q++;
		if(*p == '\0') {
			cp = --q;
			return(++p);
		}
		if(*p == '\n') {
			while(*q == '\t' || *q == ' ')	q++;
		}
		p++;
	}
}


struct label *
search(struct label *ptr)
{
	struct label	*rp;

	rp = labtab;
	while(rp < ptr) {
		if(cmp(rp->asc, ptr->asc) == 0)
			return(rp);
		rp++;
	}

	return(0);
}

void
dechain(void)
{
	struct label	*lptr;
	union reptr	*rptr, *trptr;

	for(lptr = labtab; lptr < lab; lptr++) {

		if(lptr->address == 0) {
			fprintf(stderr, "sed: Undefined label: %s\n", lptr->asc);
			exit(2);
		}

		if(lptr->chain) {
			rptr = lptr->chain;
			while(trptr = rptr->r2.lb1) {
				rptr->r2.lb1 = lptr->address;
				rptr = trptr;
			}
			rptr->r2.lb1 = lptr->address;
		}
	}
}

uchar *
ycomp(uchar *expbuf)
{
	uchar *ep, *tsp;
	int c;
	uchar	*sp;

	ep = expbuf;
	sp = cp;
	for(tsp = cp; *tsp != seof; tsp++) {
		if(*tsp == '\\')
			tsp++;
		if(*tsp == '\n' || *tsp == '\0')
			return(badp);
	}
	tsp++;

	while((c = *sp++) != seof) {
		if(c == '\\' && *sp == 'n') {
			sp++;
			c = '\n';
		}
		if((ep[c] = *tsp++) == '\\' && *tsp == 'n') {
			ep[c] = '\n';
			tsp++;
		}
		if(ep[c] == seof || ep[c] == '\0')
			return(badp);
	}
	if(*tsp != seof)
		return(badp);
	cp = ++tsp;

	for(c = 0; c<0400; c++)
		if(ep[c] == 0)
			ep[c] = c;

	return(ep + 0400);
}

