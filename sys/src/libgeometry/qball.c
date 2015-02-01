/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Ken Shoemake's Quaternion rotation controller
 */
#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>
#include <geometry.h>
#define	BORDER	4
static Point ctlcen;		/* center of qball */
static int ctlrad;		/* radius of qball */
static Quaternion *axis;	/* constraint plane orientation, 0 if none */
/*
 * Convert a mouse point into a unit quaternion, flattening if
 * constrained to a particular plane.
 */
static Quaternion mouseq(Point p){
	double qx=(double)(p.x-ctlcen.x)/ctlrad;
	double qy=(double)(p.y-ctlcen.y)/ctlrad;
	double rsq=qx*qx+qy*qy;
	double l;
	Quaternion q;
	if(rsq>1){
		rsq=sqrt(rsq);
		q.r=0.;
		q.i=qx/rsq;
		q.j=qy/rsq;
		q.k=0.;
	}
	else{
		q.r=0.;
		q.i=qx;
		q.j=qy;
		q.k=sqrt(1.-rsq);
	}
	if(axis){
		l=q.i*axis->i+q.j*axis->j+q.k*axis->k;
		q.i-=l*axis->i;
		q.j-=l*axis->j;
		q.k-=l*axis->k;
		l=sqrt(q.i*q.i+q.j*q.j+q.k*q.k);
		if(l!=0.){
			q.i/=l;
			q.j/=l;
			q.k/=l;
		}
	}
	return q;
}
void qball(Rectangle r, Mouse *m, Quaternion *result, void (*redraw)(void), Quaternion *ap){
	Quaternion q, down;
	Point rad;
	axis=ap;
	ctlcen=divpt(addpt(r.min, r.max), 2);
	rad=divpt(subpt(r.max, r.min), 2);
	ctlrad=(rad.x<rad.y?rad.x:rad.y)-BORDER;
	down=qinv(mouseq(m->xy));
	q=*result;
	for(;;){
		*m=emouse();
		if(!m->buttons) break;
		*result=qmul(q, qmul(down, mouseq(m->xy)));
		(*redraw)();
	}
}
