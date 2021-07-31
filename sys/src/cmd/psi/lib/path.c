# include <u.h>
#include <libc.h>
#include "system.h"
#ifdef Plan9
#include <libg.h>
#endif
# include "stdio.h"
# include "defines.h"
# include "object.h"
# include "path.h"
#ifndef Plan9
#include "njerq.h"
#endif
# include "graphics.h"

# define MAX(x,y)	(((x)>(y))?(x):(y))
# define PRECISION	1e-6

static	struct	element	elements[PATH_LIMIT] ;
static	struct	element	*free_element ;
static	int		hyp_err ;
static double PI_180 = M_PI/180.;
int used=0, rel=0, ct_el=0;

struct	pspoint
currentpoint(void)
{
	if ( Graphics.path.last == NULL ){
		return(zeropt);	/*printer tolerates this*/
		pserror("nocurrentpoint", "currentpoint") ;
	}
	return(Graphics.path.last->ppoint) ;
}

void
currentpointOP(void)
{
	pushpoint(itransform(currentpoint())) ;
}

void
add_element(enum elmtype type, struct pspoint p)
{
	struct	element	*element;
	switch(type){
	case PA_MOVETO :
		if ( Graphics.path.last != NULL  &&
			Graphics.path.last->type == PA_MOVETO ){
			Graphics.path.last->ppoint = p ;
			return ;
		}
		break ;

	case PA_NONE :
	case PA_LINETO :
	case PA_CURVETO :
		if ( Graphics.path.last == NULL ){
			fprintf(stderr,"type %d ",type);
			pserror("nocurrentpoint", "add_element") ;
		}
		break ;

	case PA_CLOSEPATH :
		if ( Graphics.path.last == NULL ){
			return;
			/*pserror("nocurrentpoint", "add_element CLOSEPATH");*/
		}
		if ( Graphics.path.last->type == PA_CLOSEPATH )
			return ;
		for(element=Graphics.path.last; element!=NULL;
			element=element->prev )
			if(element->type == PA_MOVETO ||
				element->type == PA_CLOSEPATH )
				break ;
		if(element->ppoint.x == Graphics.path.last->ppoint.x &&
			element->ppoint.y == Graphics.path.last->ppoint.y){
			if(Graphics.path.last->type == PA_LINETO){
				Graphics.path.last->type = PA_CLOSEPATH;
				return;
			}
		}
		if ( element == NULL )
			error("add_element nonhamiltonian path") ;
		p = element->ppoint ;
		break ;

	default :
		error("bad case in add_element") ;
	}
	if(free_element == NULL)
		pserror("limitcheck", "add_element");
	element = free_element;
	free_element = element->next;
	element->next = NULL ;	
	element->type = type ;
	element->prev = Graphics.path.last ;
	element->ppoint = p ;
	if ( Graphics.path.first == NULL )
		Graphics.path.first = element ;
	else
		element->prev->next = element ;
	Graphics.path.last = element ;
}

void
movetoOP(void)
{
	struct	pspoint point;

	point = poppoint("moveto");
	add_element(PA_MOVETO,_transform(point)) ;
}

void
rmovetoOP(void)
{
	add_element(PA_MOVETO,add_point(dtransform(poppoint("rmoveto")),currentpoint())) ;
}

void
linetoOP(void)
{
	add_element(PA_LINETO,_transform(poppoint("lineto"))) ;
}

void
rlinetoOP(void)
{
	struct 	pspoint	point;
	point = poppoint("rlineto");
	add_element(PA_LINETO,add_point(dtransform(point),currentpoint())) ;
}

void
curvetoOP(void)
{
	struct	pspoint	point1, point2, point3;

	point3 = _transform(poppoint("curveto")) ;
	point2 = _transform(poppoint("curveto")) ;
	point1 = _transform(poppoint("curveto")) ;
	add_element(PA_CURVETO,point1) ;
	add_element(PA_NONE,point2) ;
	add_element(PA_NONE,point3) ;
}

void
rcurvetoOP(void)
{
	struct	pspoint	point0, point1, point2, point3 ;

	point0 = currentpoint() ;
	point3 = add_point(point0,dtransform(poppoint("rcurveto"))) ;
	point2 = add_point(point0,dtransform(poppoint("rcurveto"))) ;
	point1 = add_point(point0,dtransform(poppoint("rcurveto"))) ;
	add_element(PA_CURVETO,point1) ;
	add_element(PA_NONE,point2) ;
	add_element(PA_NONE,point3) ;
}

void
closepathOP(void)
{
	add_element(PA_CLOSEPATH,makepoint(0.0,0.0)) ;
}
void
reversepath(struct path *path)
{
	struct element *element, *save, *last, *first=0, *next, *lastmove=0;
	struct element *none1, *none2, *curveto=0, *save1;
	int gotfirst=0, lasttype=0;

	for(element=path->first; element!= NULL; ){
		switch(element->type){
		case PA_MOVETO:
			if(!first){
				first=next=lastmove=element;
				element->prev = element->next;
			}
			else {
				if(!gotfirst){
					gotfirst=1;
					path->first = last;
					last->type = PA_MOVETO;
					first->type=first->prev->type;
					first->next=last->prev;
					last->prev = 0;
				}
				else if(lasttype != PA_CLOSEPATH){
					last->type=PA_MOVETO;
					next->type=next->prev->type;
					last->prev = lastmove;
					next->next = 0;
					lastmove->next = last;
				}
				lastmove=next;
				next = element;
				element->prev = element->next;
			}
			element=element->next;
			continue;
		case PA_CURVETO:
			lasttype=element->type;
			none1=element->next;
			none2=none1->next;
			if(!none2->next){
				none2->type = PA_MOVETO;
				none1->type = PA_CURVETO;
				element->type = PA_NONE;
				element->prev->type = PA_NONE;
			}
			else if( none2->next->type == PA_CLOSEPATH){
				none2->type = PA_LINETO;
				none1->type = PA_CURVETO;
				element->type = PA_NONE;
				element->prev->type = PA_NONE;
			}
			else {
				none2->type = PA_LINETO;
				none1->type = PA_CURVETO;
				element->type = PA_NONE;
				element->prev->type = PA_NONE;
			}
			save=element->next;
			element->next=element->prev;
			element->prev=save;
			save=none1->next;
			none1->next=none1->prev;
			none1->prev=save;
			save=none2->next;
			none2->next = none2->prev;
			none2->prev = save;
			last = none2;
			curveto=none2;
			element = save;
			continue;
		case PA_NONE:
			continue;
		case PA_LINETO:
			lasttype=element->type;
			save=element->next;
			if(curveto){
				element->next=curveto;
				curveto=0;
			}
			else element->next=element->prev;
			element->prev=save;
			last=element;
			element=save;
			continue;
		case PA_CLOSEPATH:
			lasttype=element->type;
			save=element->next;
			if(!gotfirst){
				gotfirst=1;
				path->first=element;
				if(first->type == PA_NONE){
					save1 = get_elem();
					save1->ppoint = first->ppoint;
					save1->type = PA_CLOSEPATH;
					save1->prev = first;
					save1->next = 0;
					first->next = save1;
					element->type = PA_MOVETO;
					element->next = element->prev;
					element->prev = 0;
				}
				else {
					first->type=PA_CLOSEPATH;
					element->type=PA_MOVETO;
					first->next = element->next;
					element->next = element->prev;
					element->prev=0;
				}
			}
			else{
				element->type = PA_MOVETO;
				next->type = PA_CLOSEPATH;
				element->next = element->prev;
				element->prev = lastmove;
				next->next = 0;
				lastmove->next = element;
			}
			last=element;
			element=save;
			
		}
	}
	if(!gotfirst)path->first=last;
	path->last = lastmove;
	if(lasttype != PA_CLOSEPATH){
		if(lasttype != PA_CURVETO){
			last->type=PA_MOVETO;
			last->prev = lastmove;
			next->type=PA_LINETO;
		}
		next->next = 0;
		if(lastmove && lastmove != next){
			if(curveto)lastmove->next = curveto;
			else lastmove->next = last;
		}
	}
}

void
reversepathOP(void)
{
	reversepath(&Graphics.path);
}

void
rel_path(struct element *element)
{
	struct	element	*next ;
	if(element == NULL)
		return;
	for ( ; element!=NULL ; element=next ){
		next = element->next ;
		if(element->next != NULL)
			element->next->prev = element->prev;
		if(element->prev != NULL)
			element->prev->next = element->next;
		element->next = free_element;
		element->prev = (struct element *)-1;
		free_element = element;
	}
}

void
newpathOP(void)
{
	rel_path(Graphics.path.first) ;
	Graphics.path.first = Graphics.path.last =  NULL ;
}

void
pathbboxOP(void)
{
	struct	pspoint	ll, ur ;
	struct	element	*element ;

	ll = ur = currentpoint() ;
	for (element=Graphics.path.first ; element!=NULL ; element=element->next ){
		if ( element->ppoint.x > ur.x )
			ur.x = element->ppoint.x ;
		if ( element->ppoint.y > ur.y )
			ur.y = element->ppoint.y ;
		if ( element->ppoint.x < ll.x )
			ll.x = element->ppoint.x ;
		if ( element->ppoint.y < ll.y )
			ll.y = element->ppoint.y ;
	}
	pushpoint(itransform(ll)) ;
	pushpoint(itransform(ur)) ;
}

void
pathforallOP(void)
{
	struct	object	close, curve, line, move ;

	close = procedure() ;
	curve = procedure() ;
	line = procedure() ;
	move = procedure() ;
	if ( Graphics.path.first != NULL ){
		execpush(makeoperator(loopnoop)) ;
		execpush(close) ;
		execpush(curve) ;
		execpush(line) ;
		execpush(move) ;
		execpush(makepointer((char *)Graphics.path.first)) ;
		execpush(makeoperator(xpathforall)) ;
	}
}

void
flattenpathOP(void)
{
	struct	element	*element, *save;

	for ( element=Graphics.path.first ; element!=NULL ; element=element->next )
		if ( element->type == PA_CURVETO ){
			flattencurve(element) ;
			/*fixes doug's ragged thin line problem*/
if(element->next->type == PA_CLOSEPATH &&(element->ppoint.x == element->next->ppoint.x && element->ppoint.y == element->next->ppoint.y)){
		element->type = PA_CLOSEPATH;
		if(element->next->next != NULL)
			save = element->next->next;
		else save = NULL;
		rel_elem(element->next);
		element->next = save;
		}
	}
	if(merganthal && merganthal <3 && current_type == 3)
		hsbpath(Graphics.path);
}

struct	element	*
flattencurve(struct element *element)
{
	struct	pspoint	curve[4] ;

	curve[0] = element->prev->ppoint ;
	curve[1] = element->ppoint ;
	curve[2] = element->next->ppoint ;
	curve[3] = element->next->next->ppoint ;
	rel_elem(element->next->next) ;
	rel_elem(element->next) ;
	element->type = PA_LINETO ;
	element->ppoint = curve[3] ;
	return(squash(element,curve,0.0,1.0)) ;
}

struct	element	*
squash(struct element *element, struct pspoint curve[4], double t1,double t2)
{
	struct	element	*new ;
	double tt;
	if ( maxdev(curve,t1,t2) > Graphics.flat ){
		tt = (t1 + t2)/2.;
		new = get_elem() ;
		new->type = PA_LINETO ;
		new->ppoint = bezier(curve,tt) ;
		insert(new,element) ;
		squash(element,curve,tt,t2) ;
		element = squash(new,curve,t1,tt) ;
	}
	return(element) ;
}

double
maxdev(struct pspoint p[4], double t1, double t2)
{
	int		i ;
	double		hyp, sin, cos, x, y, r1, r2, a, b, c, disc ;
	struct	pspoint	p1, p2, p3, q[4] ;

	p1 = bezier(p,t2) ;
	p2 = bezier(p,t1) ;
	hyp = hyp_point(p1, p2) ;
	if(hyp < PRECISION)
		return(0.0);
	sin = (p2.y-p1.y) / hyp ;
	cos = (p2.x-p1.x) / hyp ;
	for ( i=0 ; i<4 ; i++ ){
		x = p[i].x - p[0].x ;
		y = p[i].y - p[0].y ;
		q[i].x = cos * x + sin * y ;
		q[i].y = cos * y - sin * x ;
	}

	a = q[3].y - 3*q[2].y + 3*q[1].y ;
	b = 2*q[2].y - 4*q[1].y ;
	c = q[1].y ;
	disc = b*b - 4*a*c ;
	if ( disc < 0 ){
		disc=0.;
		/*error("maxdev failure of imagination") ;*/
	}
	if ( fabs(a) < PRECISION ){
		if ( fabs(b) < PRECISION ){
			/*fprintf(stderr,"in maxdev:b =%f\n",b) ;*/
			return(0.);
		}
		else r2 = r1 = -c / b ;
	}		
	else{
		if ( b < 0 )
			r1 = (-b + sqrt(disc)) / (2*a) ;
		else
			r1 = (-b - sqrt(disc)) / (2*a) ;
		if ( r1 == 0 )
			r2 = 1/a ;
		else
			r2 = c / (a*r1) ;
	}
	if ( t1 <= r1  &&  r1 <= t2 )
		if ( t1 <= r2  &&  r2 <= t2 ){
			p1 = bezier(q,t1);
			p2 = bezier(q,r1);
			p3 = bezier(q,r2);
			return(MAX(fabs(p1.y - p2.y), fabs(p1.y - p3.y)));
		}
		else{
			p1 = bezier(q,t1);
			p2 = bezier(q,r1);
			return(fabs(p1.y-p2.y)) ;
		}
	else
		if ( t1 <= r2  &&  r2 <= t2 ){
			p1 = bezier(q,t1);
			p2 = bezier(q,r2);
			return(fabs(p1.y-p2.y)) ;
		}
		else
			error("maxdev both roots missed interval") ;
}

struct	pspoint
bezier(struct pspoint p[4], double t)
{
	register double tt, tm;
	struct	pspoint	ret ;	
	double		t0, t1, t2, t3 ;
	tm = 1-t;
	t0 = tm * tm;
	t1 = 3 * t * t0;
	t0 *= tm;
	t3 = t * t;
	t2 = t3 * 3 * tm;
	t3 *= t;
	ret.x = t0*p[0].x + t1*p[1].x + t2*p[2].x + t3*p[3].x;
	ret.y = t0*p[0].y + t1*p[1].y + t2*p[2].y + t3*p[3].y;
	return(ret);
}

void
arcOP(void)
{
	struct object object;
	double		x, y, rad, ang1, ang2, ang ;

	object = real("arc");
	ang2 = object.value.v_real ;
	object = real("arc");
	ang1 = object.value.v_real ;
	object = real("arc");
	rad = object.value.v_real ;
	object = real("arc");
	y = object.value.v_real ;
	object = real("arc");
	x = object.value.v_real ;
	arc_line(x,y,rad,ang1) ;
	if(fabs(ang1)<PRECISION && fabs(ang2) < PRECISION){
		return;
	}
	if ( ang2 < ang1 )
		ang2 += ((int)ceil(ang1-ang2)+359) / 360 * 360 ;
	for ( ang=ang1 ; ang+90.0001<ang2 ; ang+=90 )
		arc_part(x,y,rad,ang,ang+90) ;
	arc_part(x,y,rad,ang,ang2) ;
}

void
arcnOP(void)
{
	struct object object;
	double		x, y, rad, ang1, ang2, ang ;

	object = real("arcn");
	ang2 = object.value.v_real ;
	object = real("arcn");
	ang1 = object.value.v_real ;
	object = real("arcn");
	rad = object.value.v_real ;
	object = real("arcn");
	y = object.value.v_real ;
	object = real("arcn");
	x = object.value.v_real ;

	arc_line(x,y,rad,ang1) ;
	if ( ang1 < ang2 )
		ang1 += ((int)ceil(ang2-ang1)+359) / 360 * 360 ;
	for ( ang=ang1 ; ang-90>ang2 ; ang-=90 )
		arc_part(x,y,rad,ang,ang-90) ;
	arc_part(x,y,rad,ang,ang2) ;
}

void
arc_line(double x, double y, double rad, double ang)
{
	struct	pspoint	point ;

	point = _transform(makepoint(x+rad*cos(ang*PI_180),y+rad*sin(ang*PI_180))) ;
	if ( Graphics.path.last == NULL )
		add_element(PA_MOVETO,point) ;
	else
		add_element(PA_LINETO,point) ;
}

void
arc_part(double x, double y, double rad, double ang1, double ang2)
{
	double	cang2, sang2, cang1, sang1, x1, y1, x4, y4, a, b, c, d, theta;
	double		trapezoid_area ,segment_area ;
	cang1 = cos(ang1*PI_180) ;
	sang1 = sin(ang1*PI_180) ;
	cang2 = cos(ang2*PI_180) ;
	sang2 = sin(ang2*PI_180) ;
	
	x1 = x + rad * cang1 ;
	y1 = y + rad * sang1 ;
	x4 = x + rad * cang2 ;
	y4 = y + rad * sang2 ;

	theta = ang2 - ang1 ;
	if ( theta > 180 )
		theta -= 360 ;
	if ( theta < -180 )
		theta += 360 ;
	theta *= PI_180 ;
	segment_area = 0.5 * rad * rad * (theta - sin(theta)) ;

	trapezoid_area = 0.5 * (y1+y4) * (x1-x4) ;

	a = 3.*(-cang1*sang2+cang2*sang1) / 20. ;
	b = 3.*(cang1*x1-cang1*x4-cang2*x1+cang2*x4+sang1*y1-sang1*y4
		-sang2*y1+sang2*y4) / 10. ;
	c = (x1*y1+x1*y4-x4*y1-x4*y4) / 2. ;
	c -= trapezoid_area ;
	c -= segment_area ;
	if ( a == 0 )
		d = -c/b ;
	else
		d = (-b + sqrt(b*b-4*a*c)) / (2*a) ;
	
	add_element(PA_CURVETO,_transform(makepoint(x1-d*sang1,y1+d*cang1))) ;
	add_element(PA_NONE,_transform(makepoint(x4+d*sang2,y4-d*cang2))) ;
	add_element(PA_NONE,_transform(makepoint(x4,y4))) ;
}

void
arctoOP(void)
{
	double		a, b, c, d, e, f, x, y, r, ang1, ang2 ;
	double		xt1, yt1, xt2, yt2, h01, h12;
	struct	pspoint	p0, p1, p2 ;
	struct object object;
	object = real("arcto");
	r = object.value.v_real ;
	p2 = poppoint("arcto") ;
	p1 = poppoint("arcto") ;
	p0 = itransform(currentpoint()) ;

	h01 = hyp_point(p0,p1) ;
	if ( h01 == 0.0 )
		pserror("undefinedresult", "arcto") ;

	a = (p0.y - p1.y) / h01 ;
	b = (p1.x - p0.x) / h01 ;
	c = (p0.x*p1.y - p1.x*p0.y) ;
	c /= h01 ;
	if ( a*p2.x+b*p2.y+c < 0 )
		c += r ;
	else
		c -= r ;

	h12 = hyp_point(p1,p2) ;
	if ( h12 == 0.0 )
		pserror("undefinedresult", "arcto") ;
	d = (p1.y-p2.y) / h12 ;
	e = (p2.x-p1.x) / h12 ;
	f = (p1.x*p2.y - p2.x*p1.y) ;
	f /= h12 ;
	if ( d*p0.x+e*p0.y+f < 0 )
		f += r ;
	else
		f -= r ;

	x = (c*e - b*f) / (d*b - a*e) ;
	y = (f*a - d*c) / (d*b - a*e) ;

	d = sqrt((x-p1.x)*(x-p1.x) + (y-p1.y)*(y-p1.y) - r*r) ;
	xt1 = p1.x + (d/h01) * (p0.x - p1.x) ;
	yt1 = p1.y + (d/h01) * (p0.y - p1.y) ;
	xt2 = p1.x + (d/h12) * (p2.x - p1.x) ;
	yt2 = p1.y + (d/h12) * (p2.y - p1.y) ;

	ang1 = atan2(yt1-y,xt1-x) * 180 / M_PI ;
	ang2 = atan2(yt2-y,xt2-x) * 180 / M_PI ;

	add_element(PA_LINETO,_transform(makepoint(xt1,yt1))) ;
	arc_part(x,y,r,ang1,ang2) ;

	push(makereal(xt1)) ;
	push(makereal(yt1)) ;
	push(makereal(xt2)) ;
	push(makereal(yt2)) ;
}

void
clippathOP(void)
{
	if((korean||merganthal)){
		hsbpath(Graphics.clippath);
		/*return;*/
	}
	newpathOP() ;
	Graphics.path = ncopypath(Graphics.clippath.first) ;
}
void
fillOP(void)
{
	if(charpathflg)return;
	close_components() ;
	flattenpathOP() ;
	if(Graphics.clipchanged){
		do_clip();
	}
	if(Graphics.inshow)
		sdo_fill(fillOP);
	else do_fill(fillOP) ;
	newpathOP() ;
}

void
eofillOP(void)
{
	if(charpathflg)return;
if(mdebug)printpath("before", Graphics.path, 1000, 0);
	close_components() ;
	flattenpathOP() ;
	if(Graphics.clipchanged)do_clip();
	if(Graphics.inshow)
		sdo_fill(eofillOP);
	else do_fill(eofillOP) ;
	newpathOP() ;
}

void
close_components(void)
{
	struct	element	*element, *new ;
	struct	pspoint	start ;

	if(Graphics.path.first == Graphics.path.last)return;
	for ( element=Graphics.path.first; element!=NULL; element=element->next ){
		if ( element->type == PA_MOVETO ){
			if ( element->prev != NULL && element->prev->type != PA_CLOSEPATH ){
				new = get_elem() ;
				new->type = PA_CLOSEPATH ;
				new->ppoint = start ;
				insert(new,element) ;
			}
			if(element->next == NULL){	/*last is stray moveto*/
				element->prev->next = NULL;
				Graphics.path.last = element->prev;
				continue;
			}
			else if(element->next->type == PA_MOVETO){	/*ditch dbl moveto*/
				new = element->next;
				if(element == Graphics.path.first)
					Graphics.path.first = new;
				rel_elem(element);
				element = new;
				element->prev = NULL;
			}
			start = element->ppoint ;
		}
		else if ( element->type == PA_CLOSEPATH )
			start = element->ppoint ;
		else if(element == Graphics.path.last && element->prev->type
			!= PA_CLOSEPATH){
			add_element(PA_CLOSEPATH,start);
		}
	}
}
void
strokeOP(void)
{
	if(!charpathflg)
		stroke(strokeOP) ;
}

void
strokepathOP(void)
{
	stroke(strokepathOP) ;
}

void
join(struct element *e1, struct element *e2, struct pspoint rad)
{
	switch(Graphics.linejoin){
	case JOIN_MITER :
		join_miter(e1,e2,rad) ;
		break ;

	case JOIN_ROUND :
		join_round(e1->ppoint) ;
		break ;

	case JOIN_BEVEL :
		join_bevel(e1,e2,rad) ;
		break ;

	default :
		error("bad case in join") ;
	}
}

void
outline(struct pspoint p0, struct pspoint p1, struct pspoint rad)
{
	int		diff ;
	struct	pspoint	d, p0p, p0m, p1p, p1m ;

	hyp_err = 0 ;
	d = corner(p0,p1) ;
	if ( hyp_err )
		return ;
	if(p0.y == p1.y){	/*fixes thin horizontal lines*/
		diff = (int)floor((p0.y+d.y) - (int)(p0.y-d.y));
		if(abs(diff) < 2)
			if(d.y>0.)d.y += .5;
			else d.y -= .5;
	}
	if(p0.x == p1.x){	/*fixes thin vertical lines*/
		diff = (int)floor((p0.x+d.x) - (int)(p0.x-d.x));
		if(abs(diff) < 2)
			if(d.x >0.)d.x += .5;
			else d.x -= .5;
	}
	add_element(PA_MOVETO,add_point(p0,d)) ;
	add_element(PA_LINETO,add_point(p1,d)) ;
	add_element(PA_LINETO,sub_point(p1,d)) ;
	add_element(PA_LINETO,sub_point(p0,d)) ;

	closepathOP() ;
}



struct	pspoint
corner(struct pspoint p0, struct pspoint p1)
{
	struct	pspoint	u0, u1, d ;
	double		hyp ;

	u0 = idtransform(p0) ;
	u1 = idtransform(p1) ;
	hyp = hyp_point(u0,u1) ;
	if ( hyp == 0.0 ){
		hyp_err = 1 ;
		d = makepoint(0.0,0.0) ;
		return(d) ;
	}
	d = sub_point(u1,u0) ;
	d.x *= Graphics.linewidth / hyp ;
	d.y *= Graphics.linewidth / hyp ;
	return(dtransform(rot90(d))) ;
}

void
cap(struct pspoint p0, struct pspoint p1, struct pspoint rad)
{
	struct	pspoint	c ;

	switch(Graphics.linecap){
	case CAP_BUTT :
		break ;

	case CAP_ROUND :
		join_round(p0) ;
		break ;

	case CAP_PROJSQUARE :
		hyp_err = 0 ;
		c = corner(p0,p1) ;
		if ( hyp_err )
			return ;
		outline(p0,add_point(p0,dtransform(rot90(idtransform(c)))),rad) ;
		break ;

	default :
		error("bad case in cap") ;
	}	
}

void
join_round(struct pspoint p)
{
	p = itransform(p) ;
	add_element(PA_MOVETO,_transform(add_point(p,makepoint(Graphics.linewidth,0.0)))) ;
	arc_part(p.x,p.y,Graphics.linewidth,360.0,270.0) ;
	arc_part(p.x,p.y,Graphics.linewidth,270.0,180.0) ;
	arc_part(p.x,p.y,Graphics.linewidth,180.0, 90.0) ;
	arc_part(p.x,p.y,Graphics.linewidth, 90.0,  0.0) ;
}

void
join_bevel(struct element *e1, struct element *e2, struct pspoint rad)
{
	double		hyp, t ;
	struct	pspoint	d0, d1 ;

	d0 = sub_point(e1->ppoint,e1->prev->ppoint) ;
	hyp = hyp_point(e1->ppoint,e1->prev->ppoint) ;
	if ( hyp == 0. )
		return ;
	d0.x *= rad.x / hyp ;
	d0.y *= rad.y / hyp ;

	d1 = sub_point(e2->ppoint,e2->prev->ppoint) ;
	hyp = hyp_point(e2->ppoint,e2->prev->ppoint) ;
	if ( hyp == 0. )
		return ;
	d1.x *= rad.x / hyp ;
	d1.y *= rad.y / hyp ;
	if ( d0.x*d1.y < d0.y*d1.x ){
		d0.y = -d0.y ;
		d1.y = -d1.y ;
	}
	else {
		d0.x = -d0.x ;
		d1.x = -d1.x ;
	}
	t = d0.x ;
	d0.x = d0.y ;
	d0.y = t ;
	t = d1.x ;
	d1.x = d1.y ;
	d1.y = t ;
	add_element(PA_MOVETO,e1->ppoint) ;
	add_element(PA_LINETO,add_point(e1->ppoint,d0)) ;
	add_element(PA_LINETO,add_point(e1->ppoint,d1)) ;
	closepathOP() ;
}

void
join_miter(struct element *e0, struct element *e1, struct pspoint rad)
{
	double		a, b, c, d, e, f, g;
	struct	pspoint	miter, c0, c1, d0, d1 ;

	hyp_err = 0 ;
	c0 = corner(e0->prev->ppoint, e0->ppoint) ;
	c1 = corner(e1->prev->ppoint,e1->ppoint) ;
	if( hyp_err )
		return;

	d0 = sub_point(e0->ppoint,e0->prev->ppoint) ;
	d1 = sub_point(e1->ppoint,e1->prev->ppoint) ;
	if(d0.x*d1.y > d1.x*d0.y){
		c0.x = -c0.x ;
		c0.y = -c0.y ;
		c1.x = -c1.x ;
		c1.y = -c1.y ;
	}
	a = d0.y ;
	b = -d0.x ;
	c = -(a * (e0->ppoint.x+c0.x) + b * (e0->ppoint.y+c0.y)) ;
	d = d1.y ;
	e = -d1.x ;
	f = -(d * (e1->ppoint.x+c1.x) + e * (e1->ppoint.y+c1.y)) ;
	g = d*b - a*e;
	if(fabs(g) < PRECISION){
		/*fprintf(stderr,"join_miter error g=%f\n",g);*/
		add_element(PA_MOVETO,e0->ppoint) ;
		add_element(PA_LINETO,add_point(e0->ppoint,c0)) ;
		add_element(PA_LINETO,add_point(e0->ppoint,c1)) ;
		closepathOP() ;
		return;
	}
	miter.x = (c*e - b*f) / g;
	miter.y = (f*a - d*c) / g;
				/*changed below from e1 to e0*/
				/*maybe should be both? removed 2*width*/
	a = hyp_point(itransform(e0->ppoint),itransform(miter))/Graphics.linewidth;
	b = hyp_point(itransform(e1->ppoint),itransform(miter))/Graphics.linewidth;
	if(a <= Graphics.miterlimit){
		add_element(PA_MOVETO,e0->ppoint) ;
		add_element(PA_LINETO,add_point(e0->ppoint,c0)) ;
		add_element(PA_LINETO,miter) ;
		add_element(PA_LINETO,add_point(e0->ppoint,c1)) ;
		closepathOP() ;
	}
	else if(b <= Graphics.miterlimit){
		add_element(PA_MOVETO,e1->ppoint) ;
		add_element(PA_LINETO,add_point(e1->ppoint,c0)) ;
		add_element(PA_LINETO,miter) ;
		add_element(PA_LINETO,add_point(e1->ppoint,c1)) ;
		closepathOP() ;
	}
	else join_bevel(e0,e1,rad) ;
}

struct pspoint
add_point(struct pspoint p1, struct pspoint p2)
{
	p1.x += p2.x ;
	p1.y += p2.y ;
	return(p1) ;
}

struct	pspoint
sub_point(struct pspoint p1, struct pspoint p2)
{
	p1.x -= p2.x ;
	p1.y -= p2.y ;
	return(p1) ;
}

double
hyp_point(struct pspoint p1, struct pspoint p2)
{
	struct	pspoint	d ;
	double		t ;

	d = sub_point(p1,p2) ;
	t = MAX(fabs(p1.x),fabs(p2.x)) ;
	if ( t > 0 ){
		if ( fabs(d.x)/t < PRECISION )
			d.x = 0.0 ;
	}
	else d.x = 0.0 ;
	t = MAX(fabs(p1.y),fabs(p2.y)) ;
	if ( t > 0 ){
		if ( fabs(d.y)/t < PRECISION )
			d.y = 0.0 ;
	}
	else d.y = 0.0 ;
	return(hypot(d.x,d.y)) ;
}

struct	pspoint
makepoint(double x, double y)
{
	struct	pspoint	point ;

	point.x = x ;
	point.y = y ;
	return(point) ;
}

struct	pspoint
poppoint(char *cmd)
{
	struct	pspoint	point ;
	struct object object;
	object = real(cmd);
	point.y = object.value.v_real ;
	object = real(cmd);
	point.x = object.value.v_real ;
	return(point) ;
}

void
stroke(void (*caller)(void))
{
	int		count, i, start_i ;
	double		tot, dist, offset, remaining, dash[2*DASH_LIMIT] ;
	struct path new;
	struct	element	*element, *prev ;
	struct	pspoint	q, r, rad, pt;
	count = Graphics.dash.count ;
	if ( count > 0 ){
		for ( i=0 ; i<count ; i++ )
			dash[i]=dash[i+Graphics.dash.count]=Graphics.dash.dash[i];
		count *= 2 ;
		offset = Graphics.dash.offset ;
		tot = 0 ;
		for ( i=0 ; i<count ; i++ )
			tot += dash[i] ;
		offset -= (int)(offset/tot) * tot ;
		for ( i=0 ; ; i++ )
			if ( dash[i] < offset )
				offset -= dash[i] ;
			else
				break ;
		remaining = dash[i] - offset ;
	}
	else i = 0 ;
	start_i = i ;
	pt.x = pt.y =0.;
	rad = dtransform(makepoint(Graphics.linewidth,Graphics.linewidth)) ;
	element = Graphics.path.first;
	Graphics.path.first = Graphics.path.last = NULL ;
	new.first = new.last = NULL;
	prev = NULL ;
	for  ( ; element!=NULL ;  element=element->next ){
		switch(element->type){
		case PA_MOVETO :
			stroke_comp(caller,rad,1) ;
			if(caller == (void (*)(void))strokepathOP && Graphics.path.first != NULL){
				if(new.first == NULL)new = ncopypath(Graphics.path.first);
				else appendpath(&new, Graphics.path, pt);
				newpathOP();
			}
			add_element(PA_MOVETO,element->ppoint) ;
			if ( count > 0 ){
				i = start_i ;
				remaining = dash[i] - offset ;
			}
			if ( prev != NULL )
				rel_elem(prev) ;
			prev = element ;
			continue ;

		case PA_LINETO :
			break ;

		case PA_CURVETO :
			element = flattencurve(element) ;
			break ;

		case PA_CLOSEPATH :
			break ;

		default :
			error("bad case in dash") ;
		}
		for(q=element->prev->ppoint ; count>0 &&
		    (dist=hyp_point(itransform(element->ppoint),itransform(q)))
		    >remaining ; q=r ){
			r = sub_point(element->ppoint,q) ;
			r.x *= remaining / dist ;
			r.y *= remaining / dist ;
			r = add_point(q,r) ;
/*			remaining = dash[i] ;*/
			if ( i%2 == 0 )
				add_element(PA_LINETO,r) ;
			else{
				stroke_comp(caller,rad,1) ;
				if(caller == (void (*)(void))strokepathOP && Graphics.path.first != NULL){
					if(new.first == NULL)
						new = ncopypath(Graphics.path.first);
					else appendpath(&new, Graphics.path, pt);
					newpathOP();
				}
				add_element(PA_MOVETO,r) ;
			}
			i = (i+1) % count ;
			remaining = dash[i] ;
		}
		if ( count > 0 )
			remaining -= dist ;
		r = element->ppoint ;
		if ( i%2 == 0 )
			add_element(PA_LINETO,r) ;
		else{
			stroke_comp(caller,rad,1) ;
			if(caller == (void (*)(void))strokepathOP&& Graphics.path.first != NULL){
				if(new.first == NULL)
					new = ncopypath(Graphics.path.first);
				else appendpath(&new, Graphics.path, pt);
				newpathOP();
			}
			add_element(PA_MOVETO,r) ;
		}

		if ( element->type == PA_CLOSEPATH ){
			stroke_comp(caller,rad,0) ;
			if(caller == (void (*)(void))strokepathOP&& Graphics.path.first != NULL){
				if(new.first == NULL)
					new = ncopypath(Graphics.path.first);
				else appendpath(&new, Graphics.path, pt);
				newpathOP();
			}
			add_element(PA_MOVETO,element->ppoint) ;
			if ( count > 0 ){
				i = start_i ;
				remaining = dash[i] - offset ;
			}
		}
		if ( prev != NULL )
			rel_elem(prev) ;
		prev = element ;
	}
	if ( prev != NULL )
		rel_elem(prev) ;
	stroke_comp(caller,rad,1) ;
	if(caller == (void (*)(void))strokepathOP){
		if(Graphics.path.first != NULL){
			if(new.first == NULL)return;
			else appendpath(&new,Graphics.path, pt);
		}
		Graphics.path = new;
	}
}

void
stroke_comp(void (*caller)(void), struct pspoint rad, int endflag)
{
	struct	element	*element, *first, *last ;

	if ( Graphics.path.first == NULL  ||  Graphics.path.first->next == NULL ){
		newpathOP();
		return ;
	}
	first = Graphics.path.first->next ;
	last = Graphics.path.last ;
	Graphics.path.first = Graphics.path.last = NULL ;
	for ( element=first ; element!=NULL ; element=element->next ){
		outline(element->prev->ppoint,element->ppoint,rad) ;
		if ( caller == (void (*)(void))strokeOP )
			fillOP() ;
		if ( element->next != NULL ){
			join(element,element->next,rad) ;
			if ( caller == (void (*)(void))strokeOP )
				fillOP() ;
		}
	}
	if ( endflag ){
		cap(first->prev->ppoint,first->ppoint,rad) ;
		if ( caller == (void (*)(void))strokeOP )
			fillOP() ;
		cap(last->ppoint,last->prev->ppoint,rad) ;
		if ( caller == (void (*)(void))strokeOP )
			fillOP() ;
	}
	else {
		join(last,first,rad) ;
		if ( caller == (void (*)(void))strokeOP )
			fillOP() ;
	}
	rel_path(first->prev) ;
}

void
charpathOP(void)
{
	struct	object	bool, string;

	bool = pop() ;
	if ( bool.type != OB_BOOLEAN )
		pserror("typecheck", "charpath not bool") ;
	string = pop() ;
	if ( string.type != OB_STRING )
		pserror("typecheck", "charpath not string") ;
/*	if(current_type != 1)return;*/
	charpath(string, bool);

}

struct	element	*
get_elem(void)
{
	struct	element	*element ;
	if ( free_element == NULL )
		pserror("limitcheck", "get_elem") ;
	element = free_element ;
	free_element = element->next ;
	element->next = (struct element *)-1 ;
	return(element) ;
}

void
rel_elem(struct element *element)
{
	if ( element->next != NULL )
		element->next->prev = element->prev ;
	if ( element->prev != NULL )
		element->prev->next = element->next ;
	element->next = free_element ;
	element->prev = (struct element *)-1 ;
	free_element = element ;
}

void
pathinit(void)
{
	int	i ;
	for ( i=0 ; i<NEL(elements)-1 ; i++ )
		elements[i].next = &elements[i+1] ;
	elements[i].next = NULL ;
	free_element = elements ;
}

void
insert(struct element *new, struct element *place)
{
	new->next = place ;
	new->prev = place->prev ;
	place->prev->next = new ;
	place->prev = new ;
}

struct	path
copypath(struct element *element)
{
	struct	path	oldpath, newpath ;

	oldpath = Graphics.path ;
	Graphics.path.first = Graphics.path.last = NULL ;
	for ( ; element!=NULL ; element=element->next ){
		add_element(element->type,element->ppoint) ;
	}
	newpath = Graphics.path ;
	Graphics.path = oldpath ;
	return(newpath) ;
}
struct path
ncopypath(struct element *element)
{
	struct path newpath;
	register struct element *nelement, *last;
	if(free_element == NULL)
		pserror("limitcheck", "ncopy1");

	newpath.first = last = free_element;
	free_element = last->next;
	newpath.first->type=element->type;
	newpath.first->ppoint=element->ppoint;
	newpath.first->prev = newpath.first->next = NULL;
	while((element=element->next) != NULL){
		if(free_element == NULL)
			pserror("limitcheck", "ncopy2");
		nelement = free_element;
		free_element = nelement->next;
		nelement->type = element->type;
		nelement->ppoint = element->ppoint;
		nelement->next = NULL;
		nelement->prev = last;
		nelement->prev->next = nelement;
		last = nelement;
	}
	newpath.last = last;
	return(newpath);
}
void
appendpath(struct path *path, struct path toadd, struct pspoint orig)
{
	struct element *element, *nelement, *last;
	if(toadd.first == NULL)return;
	last = path->last;
	for(element=toadd.first; element!= NULL; element=element->next){
		if(free_element == NULL)pserror("limitcheck", "appendpath");
		nelement = free_element;
		if(path->first == NULL)path->first = nelement;
		free_element = nelement->next;
		nelement->type = element->type;
		nelement->ppoint.x = element->ppoint.x + orig.x;
		nelement->ppoint.y = element->ppoint.y + orig.y;
		nelement->next = NULL;
		nelement->prev = last;
		nelement->prev->next = nelement;
		last = nelement;
	}
	path->last = last;
}

void
countfreeOP(void)
{
	int		n ;
	struct	element	*element ;

	n = 0 ;
	for ( element=free_element ; element!=NULL ; element=element->next )
		n++ ;
	push(makeint(n)) ;
}
struct	pspoint
rot90(struct pspoint p)
{
	double	t ;

	t = p.x ;
	p.x = -p.y ;
	p.y = t ;
	return(p) ;	
}
void
clean_path(struct path *path)
{
	struct element *save, *save1=0, *element;
	element = path->first;
	while(element != NULL){
		if(element->next == NULL)break;
		if(fabs(element->ppoint.x - element->next->ppoint.x)<SMALLDIF
			&& fabs(element->ppoint.y - element->next->ppoint.y)<SMALLDIF){
			if(element->type == PA_LINETO){
				save = element->next;
				element->prev->next = element->next;
				element->next->prev = element->prev;
				rel_elem(element);
				if(path->first == element)path->first = save;
				element=save;
				continue;
			}
			else if(element->type == PA_MOVETO &&
				element->next->type == PA_LINETO){
				save = element->next;
				element->next = save->next;
				save->next->prev = element;
				rel_elem(save);
				continue;
			}
			else if(element->type == PA_MOVETO &&
				element->next->type == PA_CLOSEPATH){
				if(path->first == element && path->last == element->next)break;
				if(element->prev != NULL)save1=element->prev;
				save = element->next->next;
				rel_elem(element->next);
				rel_elem(element);
				if(save == NULL){
					save1->next = NULL;
					path->last = save1;
				}
				else if(save1 != NULL){
					save1->next = save;
					save->prev = save1;
				}
				else save->prev = NULL;
				if(path->first == element)path->first = save;
				element = save;
				continue;
			}
		}
		else if(element->type == PA_MOVETO && element->next->type == PA_MOVETO){
			element->prev->next = element->next;
			element->next->prev = element->prev;
			save=element->next;
			rel_elem(element);
			element=save;
		}
		element=element->next;
	}
}
