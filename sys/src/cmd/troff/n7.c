#include "tdef.h"
#include "fns.h"
#include "ext.h"

#define GETCH gettch
Tchar	gettch(void);


/*
 * troff7.c
 * 
 * text
 */

int	brflg;

void tbreak(void)
{
	int pad, k;
	Tchar *i, j;
	int resol;
	int un0 = un;

	trap = 0;
	if (nb)
		return;
	if (dip == d && numtab[NL].val == -1) {
		newline(1);
		return;
	}
	if (!nc) {
		setnel();
		if (!wch)
			return;
		if (pendw)
			getword(1);
		movword();
	} else if (pendw && !brflg) {
		getword(1);
		movword();
	}
	*linep = dip->nls = 0;
	if (NROFF && dip == d)
		horiz(po);
	if (lnmod)
		donum();
	lastl = ne;
	if (brflg != 1) {
		totout = 0;
	} else if (ad) {
		if ((lastl = ll - un) < ne)
			lastl = ne;
	}
	if (admod && ad && (brflg != 2)) {
		lastl = ne;
		adsp = adrem = 0;
		if (admod == 1)
			un +=  quant(nel / 2, HOR);
		else if (admod == 2)
			un += nel;
	}
	totout++;
	brflg = 0;
	if (lastl + un > dip->maxl)
		dip->maxl = lastl + un;
	horiz(un);
	if (NROFF) {
		if (adrem % t.Adj)
			resol = t.Hor; 
		else 
			resol = t.Adj;
	} else
		resol = HOR;

	lastl = ne + (nwd-1) * adsp + adrem;
	for (i = line; nc > 0; ) {
		if ((cbits(j = *i++)) == ' ') {
			pad = 0;
			do {
				pad += width(j);
				nc--;
			} while ((cbits(j = *i++)) == ' ');
			i--;
			pad += adsp;
			--nwd;
			if (adrem) {
				if (adrem < 0) {
					pad -= resol;
					adrem += resol;
				} else if ((totout & 01) || adrem / resol >= nwd) {
					pad += resol;
					adrem -= resol;
				}
			}
			pchar((Tchar) WORDSP);
			horiz(pad);
		} else {
			pchar(j);
			nc--;
		}
	}
	if (ic) {
		if ((k = ll - un0 - lastl + ics) > 0)
			horiz(k);
		pchar(ic);
	}
	if (icf)
		icf++;
	else 
		ic = 0;
	ne = nwd = 0;
	un = in;
	setnel();
	newline(0);
	if (dip != d) {
		if (dip->dnl > dip->hnl)
			dip->hnl = dip->dnl;
	} else {
		if (numtab[NL].val > dip->hnl)
			dip->hnl = numtab[NL].val;
	}
	for (k = ls - 1; k > 0 && !trap; k--)
		newline(0);
	spread = 0;
}

void donum(void)
{
	int i, nw;

	nrbits = nmbits;
	nw = width('1' | nrbits);
	if (nn) {
		nn--;
		goto d1;
	}
	if (numtab[LN].val % ndf) {
		numtab[LN].val++;
d1:
		un += nw * (3 + nms + ni);
		return;
	}
	i = 0;
	if (numtab[LN].val < 100)
		i++;
	if (numtab[LN].val < 10)
		i++;
	horiz(nw * (ni + i));
	nform = 0;
	fnumb(numtab[LN].val, pchar);
	un += nw * nms;
	numtab[LN].val++;
}


void text(void)
{
	Tchar i;
	static int spcnt;

	nflush++;
	numtab[HP].val = 0;
	if ((dip == d) && (numtab[NL].val == -1)) {
		newline(1); 
		return;
	}
	setnel();
	if (ce || !fi) {
		nofill();
		return;
	}
	if (pendw)
		goto t4;
	if (pendt)
		if (spcnt)
			goto t2; 
		else 
			goto t3;
	pendt++;
	if (spcnt)
		goto t2;
	while ((cbits(i = GETCH())) == ' ') {
		spcnt++;
		numtab[HP].val += sps;
		widthp = sps;
	}
	if (nlflg) {
t1:
		nflush = pendt = ch = spcnt = 0;
		callsp();
		return;
	}
	ch = i;
	if (spcnt) {
t2:
		tbreak();
		if (nc || wch)
			goto rtn;
		un += spcnt * sps;
		spcnt = 0;
		setnel();
		if (trap)
			goto rtn;
		if (nlflg)
			goto t1;
	}
t3:
	if (spread)
		goto t5;
	if (pendw || !wch)
t4:
		if (getword(0))
			goto t6;
	if (!movword())
		goto t3;
t5:
	if (nlflg)
		pendt = 0;
	adsp = adrem = 0;
	if (ad) {
		if (nwd == 1)
			adsp = nel; 
		else 
			adsp = nel / (nwd - 1);
		adsp = (adsp / HOR) * HOR;
		adrem = nel - adsp*(nwd-1);
	}
	brflg = 1;
	tbreak();
	spread = 0;
	if (!trap)
		goto t3;
	if (!nlflg)
		goto rtn;
t6:
	pendt = 0;
	ckul();
rtn:
	nflush = 0;
}


void nofill(void)
{
	int j;
	Tchar i;

	if (!pendnf) {
		over = 0;
		tbreak();
		if (trap)
			goto rtn;
		if (nlflg) {
			ch = nflush = 0;
			callsp();
			return;
		}
		adsp = adrem = 0;
		nwd = 10000;
	}
	while ((j = (cbits(i = GETCH()))) != '\n') {
		if (j == ohc)
			continue;
		if (j == CONT) {
			pendnf++;
			nflush = 0;
			flushi();
			ckul();
			return;
		}
		j = width(i);
		widthp = j;
		numtab[HP].val += j;
		storeline(i, j);
	}
	if (ce) {
		ce--;
		if ((i = quant(nel / 2, HOR)) > 0)
			un += i;
	}
	if (!nc)
		storeline((Tchar)FILLER, 0);
	brflg = 2;
	tbreak();
	ckul();
rtn:
	pendnf = nflush = 0;
}


void callsp(void)
{
	int i;

	if (flss)
		i = flss; 
	else 
		i = lss;
	flss = 0;
	casesp1(i);
}


void ckul(void)
{
	if (ul && (--ul == 0)) {
		cu = 0;
		font = sfont;
		mchbits();
	}
	if (it && --it == 0 && itmac)
		control(itmac, 0);
}


void storeline(Tchar c, int w)
{
	if (linep >= line + lnsize - 2) {
		if (!over) {
			flusho();
			ERROR "Line overflow." WARN;
			over++;
			c = LEFTHAND;
			w = -1;
			*linep++ = c;
			*linep++ = '\n'; nc++;	/* other one comes later */
			goto s1;
		}
		return;
	}
	*linep++ = c;
s1:
	if (w == -1)
		w = width(c);
	ne += w;
	nel -= w;
	nc++;
}


void newline(int a)
{
	int i, j, nlss;
	int opn;

	if (a)
		goto nl1;
	if (dip != d) {
		j = lss;
		pchar1((Tchar)FLSS);
		if (flss)
			lss = flss;
		i = lss + dip->blss;
		dip->dnl += i;
		pchar1((Tchar)i);
		pchar1((Tchar)'\n');
		lss = j;
		dip->blss = flss = 0;
		if (dip->alss) {
			pchar1((Tchar)FLSS);
			pchar1((Tchar)dip->alss);
			pchar1((Tchar)'\n');
			dip->dnl += dip->alss;
			dip->alss = 0;
		}
		if (dip->ditrap && !dip->ditf && dip->dnl >= dip->ditrap && dip->dimac)
			if (control(dip->dimac, 0)) {
				trap++; 
				dip->ditf++;
			}
		return;
	}
	j = lss;
	if (flss)
		lss = flss;
	nlss = dip->alss + dip->blss + lss;
	numtab[NL].val += nlss;
	if (TROFF && ascii) {
		dip->alss = dip->blss = 0;
	}
	pchar1((Tchar)'\n');
	flss = 0;
	lss = j;
	if (numtab[NL].val < pl)
		goto nl2;
nl1:
	ejf = dip->hnl = numtab[NL].val = 0;
	ejl = frame;
	if (donef) {
		if ((!nc && !wch) || ndone)
			done1(0);
		ndone++;
		donef = 0;
		if (frame == stk)
			nflush++;
	}
	opn = numtab[PN].val;
	numtab[PN].val++;
	if (npnflg) {
		numtab[PN].val = npn;
		npn = npnflg = 0;
	}
nlpn:
	if (numtab[PN].val == pfrom) {
		print++;
		pfrom = -1;
	} else if (opn == pto) {
		print = 0;
		opn = -1;
		chkpn();
		goto nlpn;
	}
	if (print)
		ptpage(numtab[PN].val);	/* supposedly in a clean state so can pause */
	if (stop && print) {
		dpn++;
		if (dpn >= stop) {
			dpn = 0;
			ptpause();
		}
	}
nl2:
	trap = 0;
	if (numtab[NL].val == 0) {
		if ((j = findn(0)) != NTRAP)
			trap = control(mlist[j], 0);
	} else if ((i = findt(numtab[NL].val - nlss)) <= nlss) {
		if ((j = findn1(numtab[NL].val - nlss + i)) == NTRAP) {
			flusho();
			ERROR "Trap botch." WARN;
			done2(-5);
		}
		trap = control(mlist[j], 0);
	}
}


findn1(int a)
{
	int i, j;

	for (i = 0; i < NTRAP; i++) {
		if (mlist[i]) {
			if ((j = nlist[i]) < 0)
				j += pl;
			if (j == a)
				break;
		}
	}
	return(i);
}


void chkpn(void)
{
	pto = *(pnp++);
	pfrom = pto>=0 ? pto : -pto;
	if (pto == -32767) {
		flusho();
		done1(0);
	}
	if (pto < 0) {
		pto = -pto;
		print++;
		pfrom = 0;
	}
}


findt(int a)
{
	int i, j, k;

	k = 32767;
	if (dip != d) {
		if (dip->dimac && (i = dip->ditrap - a) > 0)
			k = i;
		return(k);
	}
	for (i = 0; i < NTRAP; i++) {
		if (mlist[i]) {
			if ((j = nlist[i]) < 0)
				j += pl;
			if ((j -= a) <= 0)
				continue;
			if (j < k)
				k = j;
		}
	}
	i = pl - a;
	if (k > i)
		k = i;
	return(k);
}


findt1(void)
{
	int i;

	if (dip != d)
		i = dip->dnl;
	else 
		i = numtab[NL].val;
	return(findt(i));
}


void eject(Stack *a)
{
	int savlss;

	if (dip != d)
		return;
	ejf++;
	if (a)
		ejl = a;
	else 
		ejl = frame;
	if (trap)
		return;
e1:
	savlss = lss;
	lss = findt(numtab[NL].val);
	newline(0);
	lss = savlss;
	if (numtab[NL].val && !trap)
		goto e1;
}


movword(void)
{
	int w;
	Tchar i, *wp;
	int savwch, hys;

	over = 0;
	wp = wordp;
	if (!nwd) {
		while (cbits(*wp++) == ' ') {
			wch--;
			wne -= sps;
		}
		wp--;
	}
	if (wne > nel && !hyoff && hyf && (!nwd || nel > 3 * sps) &&
	   (!(hyf & 02) || (findt1() > lss)))
		hyphen(wp);
	savwch = wch;
	hyp = hyptr;
	nhyp = 0;
	while (*hyp && *hyp <= wp)
		hyp++;
	while (wch) {
		if (hyoff != 1 && *hyp == wp) {
			hyp++;
			if (!wdstart || (wp > wdstart + 1 && wp < wdend &&
			   (!(hyf & 04) || wp < wdend - 1) &&		/* 04 => last 2 */
			   (!(hyf & 010) || wp > wdstart + 2))) {	/* 010 => 1st 2 */
				nhyp++;
				storeline((Tchar)IMP, 0);
			}
		}
		i = *wp++;
		w = width(i);
		wne -= w;
		wch--;
		storeline(i, w);
	}
	if (nel >= 0) {
		nwd++;
		return(0);	/* line didn't fill up */
	}
	if (TROFF)
		xbits((Tchar)HYPHEN, 1);
	hys = width((Tchar)HYPHEN);
m1:
	if (!nhyp) {
		if (!nwd)
			goto m3;
		if (wch == savwch)
			goto m4;
	}
	if (*--linep != IMP)
		goto m5;
	if (!(--nhyp))
		if (!nwd)
			goto m2;
	if (nel < hys) {
		nc--;
		goto m1;
	}
m2:
	if ((i = cbits(*(linep - 1))) != '-' && i != EMDASH) {
		*linep = (*(linep - 1) & SFMASK) | HYPHEN;
		w = width(*linep);
		nel -= w;
		ne += w;
		linep++;
	}
m3:
	nwd++;
m4:
	wordp = wp;
	return(1);	/* line filled up */
m5:
	nc--;
	w = width(*linep);
	ne -= w;
	nel += w;
	wne += w;
	wch++;
	wp--;
	goto m1;
}


void horiz(int i)
{
	vflag = 0;
	if (i)
		pchar(makem(i));
}


void setnel(void)
{
	if (!nc) {
		linep = line;
		if (un1 >= 0) {
			un = un1;
			un1 = -1;
		}
		nel = ll - un;
		ne = adsp = adrem = 0;
	}
}


getword(int x)
{
	int j, k;
	Tchar i, *wp;
	int noword;
	int obits;

	noword = 0;
	if (x)
		if (pendw) {
			*pendw = 0;
			goto rtn;
		}
	if (wordp = pendw)
		goto g1;
	hyp = hyptr;
	wordp = word;
	over = wne = wch = 0;
	hyoff = 0;
	obits = chbits;
	while (1) {	/* picks up 1st char of word */
		j = cbits(i = GETCH());
		if (j == '\n') {
			wne = wch = 0;
			noword = 1;
			goto rtn;
		}
		if (j == ohc) {
			hyoff = 1;	/* 1 => don't hyphenate */
			continue;
		}
		if (j == ' ') {
			numtab[HP].val += sps;
			widthp = sps;
			storeword(i, sps);
			continue;
		}
		break;
	}
	storeword(' ' | obits, sps);
	if (spflg) {
		storeword(' ' | obits, sps);
		spflg = 0;
	}
g0:
	if (j == CONT) {
		pendw = wordp;
		nflush = 0;
		flushi();
		return(1);
	}
	if (hyoff != 1) {
		if (j == ohc) {
			hyoff = 2;
			*hyp++ = wordp;
			if (hyp > hyptr + NHYP - 1)
				hyp = hyptr + NHYP - 1;
			goto g1;
		}
		if (j == '-' || j == EMDASH)
			if (wordp > word + 1) {
				hyoff = 2;
				*hyp++ = wordp + 1;
				if (hyp > hyptr + NHYP - 1)
					hyp = hyptr + NHYP - 1;
			}
	}
	j = width(i);
	numtab[HP].val += j;
	storeword(i, j);
g1:
	j = cbits(i = GETCH());
	if (j != ' ') {
		static char *sentchar = ".?!";	/* sentence terminators */
		if (j != '\n')
			goto g0;
		wp = wordp-1;	/* handle extra space at end of sentence */
		while (wp >= word) {
			j = cbits(*wp--);
			if (j=='"' || j=='\'' || j==')' || j==']' || j=='*' || j==DAGGER)
				continue;
			for (k = 0; sentchar[k]; k++)
				if (j == sentchar[k]) {
					spflg++;
					break;
				}
			break;
		}
	}
	*wordp = 0;
	numtab[HP].val += sps;
rtn:
	for (wp = word; *wp; wp++) {
		j = cbits(*wp);
		if (j == ' ')
			continue;
		if (!isdigit(j) && j != '-')
			break;
	}
	if (*wp == 0)	/* all numbers, so don't hyphenate */
		hyoff = 1;
	wdstart = 0;
	wordp = word;
	pendw = 0;
	*hyp++ = 0;
	setnel();
	return(noword);
}


void storeword(Tchar c, int w)
{
	if (wordp >= &word[WDSIZE - 3]) {
		if (!over) {
			flusho();
			ERROR "Word overflow." WARN;
			over++;
			c = LEFTHAND;
			w = -1;
			goto s1;
		}
		return;
	}
s1:
	if (w == -1)
		w = width(c);
	widthp = w;
	wne += w;
	*wordp++ = c;
	wch++;
}


Tchar gettch(void)
{
	extern int c_isalnum;
	Tchar i;
	int j;

	if (TROFF)
		return getch();

	i = getch();
	j = cbits(i);
	if (ismot(i) || fbits(i) != ulfont)
		return(i);
	if (cu) {
		if (trtab[j] == ' ') {
			setcbits(i, '_');
			setfbits(i, FT);	/* default */
		}
		return(i);
	}
	/* should test here for characters that ought to be underlined */
	/* in the old nroff, that was the 200 bit on the width! */
	/* for now, just do letters, digits and certain special chars */
	if (j <= 127) {
		if (!isalnum(j))
			setfbits(i, FT);
	} else {
		if (j < c_isalnum)
			setfbits(i, FT);
	}
	return(i);
}
