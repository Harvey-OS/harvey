#include <alef.h>
#include "ps.h"

	/******** rationals ********/

int
gcd(int u, int v)
{
	if(u < 0) u = -u;
	if(v < 0) v = -v;
	for(;;) {
		if(u == 0) return v;
		(u, v) = (v%u, u);
	}
	return 0;	/* to make alef happy */
}

rat
ratmk(int i, int j)
{
	int g;
	if(j == 0) return (0, 0);
	if(i == 0) return (0, 1);
	g = gcd(i, j);
	if(j > 0) return (i/g, j/g);
	return (-i/g, -j/g);
}

rat
ratneg(rat r)
{
	return (-r.num, r.den);
}

rat
ratadd(rat r, rat s)
{
	int g;
	g = gcd(r.den, s.den);
	if(g == 0) return (0, 0);
	return ratmk(r.num*(s.den/g)+s.num*(r.den/g),
		     r.den*s.den/g);
}

rat
ratsub(rat r, rat s)
{
	return ratadd(r, ratneg(s));
}

rat
ratinv(rat r)
{
	int n, d;
	(n, d) = r;
	if(n < 0) return (-d, -n);
	if(n > 0) return (d, n);
	return (0, 0);
}

rat
ratmul(rat r, rat s)
{
	int g1, g2;
	if(r.den==0 || s.den==0) return (0, 0);
	(g1, g2) = (gcd(r.num,s.den), gcd(r.den,s.num));
	return ((r.num/g1)*(s.num/g2), (r.den/g2)*(s.den/g1));
}

void
ratprint(rat r)
{
	if(r.den == 1) print("%d ", r.num);
	else print("%d/%d ", r.num, r.den);
}

	/******** power series, simple functions ********/

ps
psmk()
{
	chan(unit) req;
	chan(rat) dat;
	alloc req, dat;
	return (req, dat);
}

rat
get(ps F)
{
	F.req <-= UNIT;
	return <-F.dat;
}

void
copy(ps F, ps G)
{
	for(;;) {
		<- G.req;
		G.dat <-= get(F);
	}
}

void
xpsspy(byte* mesg, ps F, ps G)
{
	rat f;
	for(;;) {
		<-G.req;
		f = get(F);
		print("(%s %d/%d) ", mesg, f.num, f.den);
		G.dat <-= f;
	}
}

ps
psspy(byte *mesg, ps F)
{
	ps S;
	S = psmk();
	task xpsspy(mesg, F, S);
	return S;
}

void
psprint(ps F)
{
	for(;;) {
		ratprint(get(F));
		
	}
}

void
psprintn(ps F, int n)
{
	while(--n >= 0)
		ratprint(get(F));
}

void
xOnes(ps S)
{
	rat one;
	one = (1, 1);
	for(;;) {
		<-S.req;
		S.dat <-= one;
	}
}

ps
Ones()
{
	ps S;
	S = psmk();
	task xOnes(S);
	return S;
}

void
xpsmon(int n, ps M)
{
	for( ; ;n--) {
		<-M.req;
		if(n == 0)
			M.dat <-= (1, 1);
		else
			M.dat <-= (0, 1);
	}
}

ps
psmon(int n)
{
	ps M;
	M = psmk();
	task xpsmon(n, M);
	return M;
}

void
xpsadd(ps F, ps G, ps S)
{
	for(;;) {
		<-S.req;
		S.dat <-= ratadd(get(F), get(G));
	}
}

ps
psadd(ps F, ps G)
{
	ps S;
	S = psmk();
	task xpsadd(F, G, S);
	return S;
}

void
xpssub(ps F, ps G, ps S)
{
	for(;;) {
		<-S.req;
		S.dat <-= ratsub(get(F), get(G));
	}
}

ps
pssub(ps F, ps G)
{
	ps S;
	S = psmk();
	task xpssub(F, G, S);
	return S;
}
/*
void
xpscmul(rat c, ps F, ps P)
{
	for(;;) {
		<-P.req;
		P.dat <-= ratmul(c, get(F));
	}
}

ps
pscmul(rat c, ps F)
{
	ps P;
	P = psmk();
	task xpscmul(c, F, P);
	return P;
}

void
xpsxmul(ps F, ps P)
{
	<-P.req;
	P.dat <-= (0, 1);
	for(;;) {
		<-P.req;
		P.dat <-= get(F);
	}
}

ps
psxmul(ps F)
{
	ps P;
	P = psmk();
	task xpsxmul(F, P);
	return P;
}
*/
void
xpssca(rat c, int n, ps F, ps S)
{
	if(n < 0)
		exits("scaling by negative power of x");
	for(;;) {
		<-S.req;
		if(n-- > 0)
			S.dat <-= (0, 1);
		else
			S.dat <-= ratmul(c, get(F));
	}
			
}

ps
pssca(rat c, int n, ps F)
{
	ps S;
	S = psmk();
	task xpssca(c, n, F, S);
	return S;
}		

void
xpsdif(ps F, ps D)
{
	int n, g, num, den;
	<-D.req;
	get(F);
	for(n=1; ; n++) {
		(num, den) = get(F);
		g = gcd(num, den);
		D.dat <-= (num*(n/g), den/g);
		<-D.req;
	}
}

ps
psdif(ps F)
{
	ps D;
	D = psmk();
	task xpsdif(F, D);
	return D;
}

void
xpsint(ps F, ps I, rat c)
{
	int n, g, num, den;
	<-I.req;
	I.dat <-= c;
	for(n=1;;n++) {
		<-I.req;
		(num, den) = get(F);
		g = gcd(num, den);
		I.dat <-= (num/g, n*(den/g));
	}
}

ps
psint(ps F, rat c)
{
	ps I;
	I = psmk();
	task xpsint(F, I, c);
	return I;
}

	/******** power series, fanout ********/


typedef chan(unit) sig;

sig
sigmk()
{
	chan(unit) s;
	alloc s;
	return s;
}

enum State { INIT, OLD, NEW, MID, ONEV };
ps dummyps;
sig dummysig;

void xpsdup_onev(rat f, ps F, ps F0, ps F1);
void xpsdup_new(ps F, ps F0, ps F1, sig wait);
void xpsdup_mid(rat f, ps F1, sig wait, sig release);
void xpsdup_old(rat f, ps F1, sig release);
/* Initial, empty state; both streams are served */
void
xpsdup(ps F, ps F0, ps F1)
{
	rat f;
	alt {
	case <-F0.req:
		F0.dat <-= f = get(F);
		task xpsdup_onev(f, F, F0, F1);
		break;
	case <-F1.req:
		F1.dat <-= f = get(F);
		task xpsdup_onev(f, F, F1, F0);
	}
}
/* Holds one value for stream F1; both streams are served */
void
xpsdup_onev(rat f, ps F, ps F0, ps F1)
{
	sig signal;
	signal = sigmk();
	alt {
	case <-F0.req:
		task xpsdup_new(F, F0, F1, signal);
		task xpsdup_old(f, F1, signal);
		break;
	case <-F1.req:
		F1.dat <-= f;
		task xpsdup(F, F0, F1);
	}
}
/* Holds the newest of several values for F1; F0 is served */
void
xpsdup_new(ps F, ps F0, ps F1, sig wait)
{
	sig signal;
	rat f;
	f = get(F);
	F0.dat <-= f;
	signal = sigmk();
	alt {
	case <-wait:
		unalloc wait;
		task xpsdup_onev(f, F, F0, F1);
		break;
	case <-F0.req:
		task xpsdup_new(F, F0, F1, signal);
		task xpsdup_mid(f, F1, wait, signal);
	}
}
/* Holds middle-aged value for F1, waiting to become oldest */
void
xpsdup_mid(rat f, ps F1, sig wait, sig release)
{
	<-wait;
	unalloc wait;
	task xpsdup_old(f, F1, release);
}
/* Holds oldest value for F1; F1 is served */
void
xpsdup_old(rat f, ps F1, sig release)
{
	<-F1.req;
	F1.dat <-= f;
	release <-= UNIT;
}

(ps, ps)
psdup(ps F)
{
	ps F0, F1;
	(F0, F1) = (psmk(),psmk());
	task xpsdup(F, F0, F1);
	return (F0, F1);
}

void
xpsmul(ps F, ps G, ps P)
{
	rat f, g;
	ps F0, F1, G0, G1;
	ps fG, gF, xFG;
	<-P.req;
	f = get(F);
	g = get(G);
	(F0, F1) = psdup(F);
	(G0, G1) = psdup(G);
	P.dat <-= ratmul(f, g);
	fG = pscmul(f, G0);
	gF = pscmul(g, F0);
	xFG = psxmul(psmul(F1, G1));
	for(;;) {
		<-P.req;
		P.dat <-= ratadd(ratadd(get(fG), get(gF)), get(xFG));
	}
}

ps
psmul(ps F, ps G)
{
	ps P;
	P = psmk();
	task xpsmul(F, G, P);
	return P;
}

void
xpscom(ps F, ps G, ps S)
{
	rat g;
	ps G0, G1;
	ps Sbar;
	(G0, G1) = psdup(G);
	<-S.req;
	S.dat <-= get(F);
	g = get(G0);
	if(g.num != 0) {
		print("2nd arg of com has nonzero const term\n");
		exits("");
	}
	Sbar = psmul(G0, pscom(F, G1));
	for(;;) {
		<-S.req;
		S.dat <-= get(Sbar);
	}
}

ps
pscom(ps F, ps G)	/* F(G) where G(0) = 0 */
{
	ps S;
	S = psmk();
	task xpscom(F, G, S);
	return S;
}

ps
psexp(ps F)	/* exp(F) where F(0)== 0 */
{
	ps X, I;
	ps X0, X1;
	X = psmk();
	(X0, X1) = psdup(X);
	I = psint(psmul(X0, psdif(F)), (1, 1));
	task copy(I, X);
	return X1;
}

void
xpsinv(ps F, ps G, ps GG0)
{
	rat g;
	ps FG, gFG;
	<- G.req;
	g = ratinv(get(F));
	G.dat <-= g;
	FG = psmul(F, GG0);
	gFG = pscmul(ratneg(g), FG);
	for(;;) {
		<-G.req;
		G.dat <-= get(gFG);
	}
}

ps
psinv(ps F)		/* 1/F */
{
	ps G, G0, G1;
	G = psmk();
	(G0, G1) = psdup(G);
	task xpsinv(F, G, G0);
	return G1;
}

void
xpsrev(ps F, ps G, ps G0)	/* reversion, F(0)==0, F'(0)!=0 */
{
	rat g;
	ps G2, G3, Gbar0, Gbar1;
	ps Gbarsq, Gb, R;
	<-G.req;
	get(F);			/* discard F(0) */
	G.dat <-= (0, 1);
	g = ratinv(get(F));
	<-G.req;
	G.dat <-= g;
	(G2, G3) = psdup(G0);
	get(G2);   
	(Gbar0, Gbar1) = psdup(G2);
	Gbarsq = psmul(Gbar0, Gbar1);
	Gb = psmul(Gbarsq, pscom(F, G3));
	R = pscmul(ratneg(g), Gb);
	for(;;) {
		<-G.req;
		G.dat <-= get(R);
	}
}

ps
psrev(ps F)
{
	ps G, G0, G1;
	G = psmk();
	(G0, G1) = psdup(G);
	task xpsrev(F, G, G0);
	return G1;
}

ps
pssqu(ps F)
{
	ps F0, F1;
	(F0, F1) = psdup(F);
	return psmul(F0, F1);
}
