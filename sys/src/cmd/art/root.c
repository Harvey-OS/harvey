/*
 * Use Sturm sequences to isolate polynomial roots.
 * A Sturm sequence for a polynomial a(x) is any sequence of polynomials
 * a[0](x),...,a[n](x) with
 *	a[0](x)=a(x)
 *	a[1](x)=a'(x)
 *	a[i+2](x)=(a[i+1](x)*q[i+1](x)-b[i]*a[i](x))/c[i]
 * where b[i] and c[i] are positive real numbers, and degree(a[n](x))==0.
 *
 * Note:
 *	a[i+2](x)=-a[i](x) mod a[i+1](x)
 * (where p(x) mod q(x) is the remainder after `synthetic division')
 * works just fine and minimizes n.
 *
 * Now, we can define
 *	schange(a, x)
 * to be the number of times that the sign of the sequence a[i](x) changes from +
 * to - or vice versa (ignoring zero terms.)
 * Sturm's theorem is that if a(x) is square-free, the number of real zeros
 * of a(x) on the interval (x0,x1] is schange(a, x0)-schange(a, x1).
 */
#include "art.h"
#define	DEGREE	5
Flt rem[DEGREE+1][DEGREE+1];
int Drem[DEGREE+1];
int nrem;
Flt quo[DEGREE+1][DEGREE+1];
int Dquo[DEGREE+1];
Flt evalrem(), evalquo();
Flt lastroot=-1;
int simple;
int nroot, nhard;
#include <sys/types.h>
#include <sys/times.h>
time_t treg;
struct tms areg, breg;
/*
 * Find the all roots of a(x) on [0,1].
 * Return number of roots.
 * Method:
 *	1) compute a Sturm sequence for a(x)
 *	2) use Newton iteration starting at the root saved from the last
 *	   invocation to find a root.  The Sturm sequence can verify that
 *	   it's the correct root.
 *	3) If that doesn't get the root, use the Sturm sequence to direct
 *	   binary subdivision to isolate the smallest root on the interval.
 *	4) Having bracketed the root, use a hybrid Newton/Regula Falsi iteration
 *	   to home in on the root.
 */
Flt *root;
int roots(Flt a[], int adeg, Flt r[]){
	Flt x0, x1, x, y0, y1, y, d;
	register n0, n1, n;
	/*
	 * get the Sturm sequence of a
	 */
	for(n=0;n<=adeg;n++) rem[0][n]=a[n];
	Drem[0]=adeg;
	for(;;){
		for(n=1;n<=Drem[0];n++)
			rem[1][n-1]=n*rem[0][n];
		Drem[1]=Drem[0]-1;
		simple=1;
		for(nrem=1;Drem[nrem]!=0;nrem++){
			polydivnrem(rem[nrem-1], Drem[nrem-1],
				rem[nrem], Drem[nrem],
				quo[nrem], &Dquo[nrem],
				rem[nrem+1], &Drem[nrem+1]);
			if(Dquo[nrem]!=1) simple=0;
		}
		/*
		 * If the rem[n] is zero, a is not square-free, and
		 * rem[n-1] is gcd(a, a').  In that case, dividing
		 * rem[0] by rem[n-1] will reduce the multiplicities
		 * of all roots to 1, and we can go back and try again.
		 */
		if(rem[nrem][0]!=0) break;
		Drem[0]=polydiv(rem[0], Drem[0], rem[nrem-1], Drem[nrem-1],
				rem[0]);
	}
	/*
	 * Isolate all roots between 0 and 1
	 */
	root=r;
	isolate(0., schange(0.), 1., schange(1.));
	return root-r;
}
void isolate(Flt x0, int n0, Flt x1, int n1){
	Flt x;
	int n;
	while(n0-n1>1){
		x=.5*(x0+x1);
		n=schange(x);
		isolate(x0, n0, x, n);
		x0=x;
		n0=n;
	}
	if(n0==n1) return;
	/*
	 * iterate to the root
	 */
	y0=evalrem(0, x0);
	if(fabs(y0)<EPS) return;	/* this root counted in other subdivision */
	y1=evalrem(0, x1);
	if(fabs(y1)<EPS){
		*root++=x1;
		return;
	}
	if(y0*y1>=0){
		msg("Root not bracketed x0=%g, y0=%g, x1=%g, y1=%g", x0, y0, x1, y1);
		return;
	}
	/*
	 * Pegasus iteration
	 */
	x=x0;
	y=y0;
	while(fabs(x1-x0)>EPS){
		x=x0-y0*(x1-x0)/(y1-y0);
		y=evalrem(0, x);
		if(fabs(y)<EPS){
			*root++=x;
			return;
		}
		if(y*y0<=0.){
			x1=x0;
			y1=y0;
		}
		else
			y1 *= y0/(y0+y);
		x0=x;
		y0=y;
	}
	*root++=x0-y0*(x1-x0)/(y1-y0);
}
/*
 * r=-a mod b
 * q=a/b
 */
polydivnrem(a, adeg, b, bdeg, q, qdeg, r, rdeg)
Flt a[], b[], q[], r[];
int *rdeg, *qdeg;
{
	register Flt *rp, *erp;
	polydivrem(a, adeg, b, bdeg, q, qdeg, r, rdeg);
	erp=&r[*rdeg];
	for(rp=r;rp<=erp;rp++) *rp=-*rp;
}
/*
 * q=a/b
 */
polydiv(a, adeg, b, bdeg, q)
Flt a[], b[], q[];
{
	Flt r[DEGREE+1], tq[DEGREE+1];
	int rdeg, qdeg;
	register Flt *qp, *tqp, *eqp;
	polydivrem(a, adeg, b, bdeg, tq, &qdeg, r, &rdeg);
	eqp=&q[qdeg];
	for(qp=q,tqp=tq;qp<=eqp;) *qp++=*tqp++;
	return qdeg;
}
/*
 * polynomial division of a by b.
 * quotient in q, remainder in r.
 */
polydivrem(a, adeg, b, bdeg, q, qdeg, r, rdeg)
Flt a[], b[], q[], r[];
int adeg, bdeg, *qdeg, *rdeg;
{
	Flt quot;
	register i, qterm, rterm;
	while(adeg>0 && a[adeg]==0.) --adeg;
	while(bdeg>0 && b[bdeg]==0.) --bdeg;
	for(i=0;i<=adeg;i++) r[i]=a[i];
	rterm=adeg;
	qterm=adeg-bdeg;
	*qdeg=qterm;
	while(rterm>=bdeg){
		quot=q[qterm]=r[rterm]/b[bdeg];
		for(i=0;i<=bdeg;i++)
			r[qterm+i]-=b[i]*quot;
		--qterm;
		--rterm;
		while(rterm>0 && r[rterm]==0) --rterm;
	}
	*rdeg=rterm;
}
ppoly(poly, n)
Flt *poly;
{
	register plus=0, i;
	for(i=n;i>=0;--i) if(poly[i]!=0){
		if(plus){
			if(poly[i]>=0) putchar('+');
		}
		else plus++;
		if(poly[i]==-1 && i!=0) putchar('-');
		else if(poly[i]!=1 || i==0) printf("%.20g", poly[i]);
		switch(i){
		default: printf("x^%d", i); break;
		case 1: putchar('x'); break;
		case 0: break;
		}
	}
	if(!plus) putchar('0');
	putchar('\n');
}
/*
 * Use Horner's rule to evaluate rem[n] at x
 */
Flt evalrem(n, x)
Flt x;
{
	register Flt *d, *d0=&rem[n][0];
	register Flt f=0;
	for(d=d0+Drem[n];d>=d0;--d) f=f*x+*d;
	return f;
}
/*
 * Use Horner's rule to evaluate quo[n] at x
 */
Flt evalquo(n, x)
Flt x;
{
	register Flt *d, *d0=&quo[n][0];
	register Flt f=0;
	for(d=d0+Dquo[n];d>=d0;--d) f=f*x+*d;
	return f;
}
/*
 * Count Sturm sequence sign changes at x
 */
schange(x)
Flt x;
{
	register i, n=0;
	register Flt rip1, ri, rim1;
	rip1=evalrem(nrem, x);
	ri=evalrem(nrem-1, x);
	if(simple){
		for(i=nrem-1;i!=0;--i){
			if(rip1!=0) if(rip1<0?ri>0:ri<0) n++;
			rim1=ri*(quo[i][0]+x*quo[i][1])-rip1;
			rip1=ri;
			ri=rim1;
		}
	}
	else{
		for(i=nrem-1;i!=0;--i){
			if(rip1!=0) if(rip1<0?ri>0:ri<0) n++;
			rim1=ri*evalquo(i, x)-rip1;
			rip1=ri;
			ri=rim1;
		}
	}
	if(rip1!=0) if(rip1<0?ri>0:ri<0) n++;
	return n;
}
