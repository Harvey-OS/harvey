#include "sam.h"
/*
 * GROWDATASIZE must be big enough that all errors go out as Hgrowdata's,
 * so they will be scrolled into visibility in the ~~sam~~ window (yuck!).
 */
#define	GROWDATASIZE	50	/* if size is > this, send data with grow */

void	rcut(List*, Posn, Posn);
int	rterm(List*, Posn);
void	rgrow(List*, Posn, Posn);

void
toterminal(File *f, int toterm)
{
	Buffer *t = f->transcript;
	Posn n, p0, p1, p2, delta = 0, deltacmd = 0;
	Range r;
	union{
		union	Hdr g;
		Rune	buf[8+GROWDATASIZE];
	}hdr;
	Posn growpos, grown;

	if(f->rasp == 0)
		return;
	if(f->marked)
		p0 = f->markp+sizeof(Mark)/RUNESIZE;
	else
		p0 = 0;
	grown = 0;
	noflush = 1;
	SET(growpos);
	while(Bread(t, (Rune*)&hdr, sizeof(hdr)/RUNESIZE, p0) > 0){
		switch(hdr.g.cs.c){
		default:
			fprint(2, "char %c %.2ux\n", hdr.g.cs.c, hdr.g.cs.c);
			panic("unknown in toterminal");

		case 'd':
			if(grown){
				outTsll(Hgrow, f->tag, growpos, grown);
				grown = 0;
			}
			p1 = hdr.g.cll.l;
			p2 = hdr.g.cll.l1;
			if(p2 <= p1)
				panic("toterminal delete 0");
			if(f==cmd && p1<cmdpt){
				if(p2 <= cmdpt)
					deltacmd -= (p2-p1);
				else
					deltacmd -= cmdpt-p1;
			}
			p1 += delta;
			p2 += delta;
			p0 += sizeof(struct _cll)/RUNESIZE;
			if(toterm)
				outTsll(Hcut, f->tag, p1, p2-p1);
			rcut(f->rasp, p1, p2);
			delta -= p2-p1;
			break;

		case 'f':
			if(grown){
				outTsll(Hgrow, f->tag, growpos, grown);
				grown = 0;
			}
			n = hdr.g.cs.s;
			p0 += sizeof(struct _cs)/RUNESIZE + n;
			break;

		case 'i':
			n = hdr.g.csl.s;
			p1 = hdr.g.csl.l;
			p0 += sizeof(struct _csl)/RUNESIZE + n;
			if(n <= 0)
				panic("toterminal insert 0");
			if(f==cmd && p1<cmdpt)
				deltacmd += n;
			p1 += delta;
			if(toterm){
				if(n>GROWDATASIZE || !rterm(f->rasp, p1)){
					rgrow(f->rasp, p1, n);
					if(grown && growpos+grown!=p1){
						outTsll(Hgrow, f->tag, growpos, grown);
						grown = 0;
					}
					if(grown)
						grown += n;
					else{
						growpos = p1;
						grown = n;
					}
				}else{
					Rune *rp;
					if(grown){
						outTsll(Hgrow, f->tag, growpos, grown);
						grown = 0;
					}
					rp = hdr.buf+sizeof(hdr.g.csl)/RUNESIZE;
					rgrow(f->rasp, p1, n);
					r = rdata(f->rasp, p1, n);
					if(r.p1!=p1 || r.p2!=p1+n)
						panic("rdata in toterminal");
					outTsllS(Hgrowdata, f->tag, p1, n, tmprstr(rp, n));
				}
			}else{
				rgrow(f->rasp, p1, n);
				r = rdata(f->rasp, p1, n);
				if(r.p1!=p1 || r.p2!=p1+n)
					panic("rdata in toterminal");
			}
			delta += n;
			break;
		}
	}
	if(grown)
		outTsll(Hgrow, f->tag, growpos, grown);
	if(toterm)
		outTs(Hcheck0, f->tag);
	outflush();
	noflush = 0;
	if(f == cmd){
		cmdpt += deltacmd+cmdptadv;
		cmdptadv = 0;
	}
}

#define	M	0x80000000L
#define	P(i)	r->longptr[i]
#define	T(i)	(P(i)&M)	/* in terminal */
#define	L(i)	(P(i)&~M)	/* length of this piece */

void
rcut(List *r, Posn p1, Posn p2)
{
	Posn p, x;
	int i;

	if(p1 == p2)
		panic("rcut 0");
	for(p=0,i=0; i<r->nused && p+L(i)<=p1; p+=L(i++))
		;
	if(i == r->nused)
		panic("rcut 1");
	if(p<p1){	/* chop this piece */
		if(p+L(i) < p2){
			x = p1-p;
			p += L(i);
		}else{
			x = L(i)-(p2-p1);
			p = p2;
		}
		if(T(i))
			P(i) = x|M;
		else
			P(i) = x;
		i++;
	}
	while(i<r->nused && p+L(i)<=p2){
		p += L(i);
		dellist(r, i);
	}
	if(p<p2){
		if(i == r->nused)
			panic("rcut 2");
		x = L(i)-(p2-p);
		if(T(i))
			P(i) = x|M;
		else
			P(i) = x;
	}
	/* can we merge i and i-1 ? */
	if(i>0 && i<r->nused && T(i-1)==T(i)){
		x = L(i-1)+L(i);
		dellist(r, i--);
		if(T(i))
			P(i)=x|M;
		else
			P(i)=x;
	}
}

void
rgrow(List *r, Posn p1, Posn n)
{
	Posn p;
	int i;

	if(n == 0)
		panic("rgrow 0");
	for(p=0,i=0; i<r->nused && p+L(i)<=p1; p+=L(i++))
		;
	if(i == r->nused){	/* stick on end of file */
		if(p!=p1)
			panic("rgrow 1");
		if(i>0 && !T(i-1))
			P(i-1)+=n;
		else
			inslist(r, i, n);
	}else if(!T(i))		/* goes in this empty piece */
		P(i)+=n;
	else if(p==p1 && i>0 && !T(i-1))	/* special case; simplifies life */
		P(i-1)+=n;
	else if(p==p1)
		inslist(r, i, n);
	else{			/* must break piece in terminal */
		inslist(r, i+1, (L(i)-(p1-p))|M);
		inslist(r, i+1, n);
		P(i) = (p1-p)|M;
	}
}

int
rterm(List *r, Posn p1)
{
	Posn p;
	int i;

	for(p = 0,i = 0; i<r->nused && p+L(i)<=p1; p+=L(i++))
		;
	if(i==r->nused && (i==0 || !T(i-1)))
		return 0;
	return T(i);
}

Range
rdata(List *r, Posn p1, Posn n)
{
	Posn p;
	int i;
	Range rg;

	if(n==0)
		panic("rdata 0");
	for(p = 0,i = 0; i<r->nused && p+L(i)<=p1; p+=L(i++))
		;
	if(i==r->nused)
		panic("rdata 1");
	if(T(i)){
		n-=L(i)-(p1-p);
		if(n<=0){
			rg.p1 = rg.p2 = p1;
			return rg;
		}
		p+=L(i++);
		p1 = p;
	}
	if(T(i) || i==r->nused)
		panic("rdata 2");
	if(p+L(i)<p1+n)
		n = L(i)-(p1-p);
	rg.p1 = p1;
	rg.p2 = p1+n;
	if(p!=p1){
		inslist(r, i+1, L(i)-(p1-p));
		P(i)=p1-p;
		i++;
	}
	if(L(i)!=n){
		inslist(r, i+1, L(i)-n);
		P(i)=n;
	}
	P(i)|=M;
	/* now i is set; can we merge? */
	if(i<r->nused-1 && T(i+1)){
		P(i)=(n+=L(i+1))|M;
		dellist(r, i+1);
	}
	if(i>0 && T(i-1)){
		P(i)=(n+L(i-1))|M;
		dellist(r, i-1);
	}
	return rg;
}
