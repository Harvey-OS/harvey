/*
 * commands dealing with alignments
 */
#include "art.h"
#define	NASTR	10
char *mslopes[NALIGN+3]={ "slopes", " measure", 0 };
char *mangles[NALIGN+3]={ "angles", " measure", 0 };
char *mparallels[NALIGN+3]={ "parallels", " measure", 0 };
char *mcircles[NALIGN+3]={ "circles", " measure", 0 };
int nalign[4];
char **amenu[4]={mslopes, mangles, mparallels, mcircles};
Flt slope[]={
	  0.0,
	 15.0,
	 30.0,
	 45.0,
	 60.0,
	 72.0,
	 90.0,
	108.0,
	120.0,
	135.0,
	150.0,
	-1
};
Flt distance[]={
	0.05,
	0.10,
	0.25,
	0.50,
	1.00,
	-1
};
#define	INVALID	-1e6
Flt Mslope(void){
	Flt s;
	if(narg<2){
		msg("first pick two points, then measure slope");
		return INVALID;
	}
	s=angle(Dpt(1, 0), dsub(arg[0], arg[1]));
	while(s<0) s+=180.;
	if(s>=180.) s-=180.;
	msg("slope %.2f", s);
	narg=0;
	return s;
}
Flt Mangle(void){
	Flt a;
	if(narg<3){
		msg("first pick three points, then measure angle");
		return INVALID;
	}
	a=angle(dsub(arg[2], arg[1]), dsub(arg[0], arg[1]));
	while(a<0) a+=180.;
	if(a>=180.) a-=180.;
	msg("angle %.2f", a);
	narg=0;
	return a;
}
Flt Mdist(void){
	Flt d;
	if(narg<2){
		msg("first pick two points, then measure distance");
		return INVALID;
	}
	d=dist(arg[1], arg[0]);
	msg("distance %.2f", d);
	narg=0;
	return d;
}
/*
 * Process a hit on some alignment menu.
 *	button is the number of the button hit
 *	measure is a routine to measure a new alignment
 *	kind is the kind of alignment -- subsequent arguments
 *	are more kinds that should have their menus augmented when this one does.
 */
#define	TOL	.01	/* should be a parameter */
#include <stdarg.h>
void Oalign(int button, Flt (*measure)(void), int kind, ...){
	Flt v;
	int i, n, ins, k;
	va_list more;
	Align *bp;
	if(button==0){
		v=(*measure)();
		if(v==INVALID) return;
		n=nalign[kind];
		ins=0;
		for(i=0;i!=n;i++){
			if(fabs(v-align[kind][i].value)<TOL){
				button=i+1;
				align[kind][i].active=0; /* code at Ordinary will set */
				goto Ordinary;
			}
			if(v>=align[kind][i].value) ins=i+1;
		}
		va_start(more, kind);
		for(k=kind;k!=-1;k=va_arg(more, int)){
			n=nalign[k];
			++nalign[k];
			for(i=n;i!=ins;--i) align[k][i]=align[k][i-1];
			amenu[k][n+2]=align[k][n].button;
			align[k][ins].value=v;
			align[k][ins].active=0;
			sprint(align[k][ins].button, " %7.2f", v);
		}
		va_end(more);
		button=ins+1;
	}
Ordinary:
	bp=&align[kind][button-1];
	bp->active=!bp->active;
	sprint(bp->button, "%c%7.2f", bp->active?'*':' ', bp->value);
	reheat();
}
void Oslope(int n){
	Oalign(n, Mslope, SLOPE, ANGLE, -1);
}
void Oangle(int n){
	Oalign(n, Mangle, ANGLE, SLOPE, -1);
}
void Oparallel(int n){
	Oalign(n, Mdist, PARA, CIRC, -1);
}
void Ocirc(int n){
	Oalign(n, Mdist, CIRC, PARA, -1);
}
void ainit(void){
	int i, j;
	for(i=0;slope[i]>=0;i++) for(j=SLOPE;j<=ANGLE;j++){
		align[j][i].value=slope[i];
		sprint(align[j][i].button, " %7.2f", slope[i]);
		amenu[j][i+2]=align[j][i].button;
	}
	for(j=SLOPE;j<=ANGLE;j++) nalign[j]=i;
	for(i=0;distance[i]>=0;i++) for(j=PARA;j<=CIRC;j++){
		align[j][i].value=distance[i];
		sprint(align[j][i].button, " %7.2f", distance[i]);
		amenu[j][i+2]=align[j][i].button;
	}
	for(j=PARA;j<=CIRC;j++) nalign[j]=i;
}
