#include <u.h>
#include <libc.h>
#include "system.h"
# include "stdio.h"
# include "defines.h"
# include "object.h"
# include "path.h"
#ifndef Plan9
#include "njerq.h"
#endif
# include "graphics.h"


void
matrixOP(void)
{
	push(makematrix(makearray(CTM_SIZE,XA_LITERAL),1.0,0.0,0.0,1.0,0.0,0.0)) ;
}

void
identmatrixOP(void)
{
	push(makematrix(pop(),1.0,0.0,0.0,1.0,0.0,0.0)) ;
}

void
currentmatrixOP(void)
{
	push(makematrix(pop(),G[0],G[1],G[2],G[3],G[4],G[5])) ;
}

void
setmatrixOP(void)
{
	int		i ;
	struct	object	matrix;
	matrix = realmatrix(pop()) ;
	for ( i=0 ; i<CTM_SIZE ; i++ )
		G[i] = E(matrix,i) ;
	fontchanged = 1;
}

void
translateOP(void)
{
	double		tx, ty ;
	struct	object	object, junk;
	object = pop() ;
	switch(object.type){
	case OB_ARRAY :
		junk=real("translate");
		ty = junk.value.v_real ;
		junk=real("translate");
		tx = junk.value.v_real ;
		push(makematrix(object,1.0,0.0,0.0,1.0,tx,ty)) ;
		return;

	case OB_INT :
		push(object) ;
		object = real("translate") ;
	case OB_REAL :
		ty = object.value.v_real ;
		junk=real("translate");
		tx = junk.value.v_real ;
		G[4] += tx*G[0] + ty*G[2] ;
		G[5] += tx*G[1] + ty*G[3] ;
		return;

	default :
		pserror("typecheck", "translate") ;
	}
}

void
scaleOP(void)
{
	double		sx, sy ;
	struct	object	object, junk;

	object = pop() ;
	switch(object.type){
	case OB_ARRAY :
		junk=real("scale");
		sy = junk.value.v_real ;
		junk=real("scale");
		sx = junk.value.v_real ;
		push(makematrix(object,sx,0.0,0.0,sy,0.0,0.0)) ;
		return;

	case OB_INT :
		push(object) ;
		object = real("scale") ;
	case OB_REAL :
		sy = object.value.v_real ;
		junk=real("scale");
		sx = junk.value.v_real ;
		G[0] *= sx ;
		G[1] *= sx ;
 			G[2] *= sy ;
		G[3] *= sy ;
		fontchanged = 1;
		return;

	default :
		pserror("typecheck", "scale") ;
	}
}

void
rotateOP(void)
{
	double		c, s, t0, t1, t2, t3;
	struct	object	object, junk;
	object = pop() ;
	switch(object.type){
	case OB_ARRAY :
		dupOP() ;
		cosOP() ;
		junk=real("rotate");
		c = junk.value.v_real ;
		sinOP() ;
		junk=real("rotate");
		s = junk.value.v_real ;
		push(makematrix(object,c,s,-s,c,0.0,0.0)) ;
		return;

	case OB_INT :
		object.type = OB_REAL ;
		object.value.v_real = (double)object.value.v_int ;
	case OB_REAL :
		push(object) ;
		dupOP() ;
		cosOP() ;
		junk=real("rotate");
		c = junk.value.v_real ;
		sinOP() ;
		junk=real("rotate");
		s = junk.value.v_real ;
		t0 = c * G[0] + s * G[2] ;
		t1 = c * G[1] + s * G[3] ;
		t2 = -s * G[0] + c * G[2] ;
		t3 = -s * G[1] + c * G[3] ;
		G[0] = t0 ;
		G[1] = t1 ;
		G[2] = t2 ;
		G[3] = t3 ;
		fontchanged = 1;
		return;

	default :
		pserror("typecheck", "rotate") ;
	}
}

void
concatOP(void)
{
	double		t0, t1, t2, t3, t4, t5 ;
	struct	object	object;

	object = realmatrix(pop()) ;
	t0 = E(object,0)*G[0] + E(object,1)*G[2] ;
	t1 = E(object,0)*G[1] + E(object,1)*G[3] ;
	t2 = E(object,2)*G[0] + E(object,3)*G[2] ;
	t3 = E(object,2)*G[1] + E(object,3)*G[3] ;
	t4 = E(object,4)*G[0] + E(object,5)*G[2] + G[4] ;
	t5 = E(object,4)*G[1] + E(object,5)*G[3] + G[5] ;
	G[0] = t0 ;
	G[1] = t1 ;
	G[2] = t2 ;
	G[3] = t3 ;
	G[4] = t4 ;
	G[5] = t5 ;
	fontchanged = 1;
}

void
concatmatrixOP(void)
{
	struct	object	m1, m2, m3 ;
	double		t0, t1, t2, t3, t4, t5 ;

	m3 = pop() ;
	m2 = realmatrix(pop()) ;
	m1 = realmatrix(pop()) ;
	t0 = E(m1,0) * E(m2,0) + E(m1,1) * E(m2,2) ;
	t1 = E(m1,0) * E(m2,1) + E(m1,1) * E(m2,3) ;
	t2 = E(m1,2) * E(m2,0) + E(m1,3) * E(m2,2) ;
	t3 = E(m1,2) * E(m2,1) + E(m1,3) * E(m2,3) ;
	t4 = E(m1,4) * E(m2,0) + E(m1,5) * E(m2,2) + E(m2,4) ;
	t5 = E(m1,4) * E(m2,1) + E(m1,5) * E(m2,3) + E(m2,5) ;
	push(makematrix(m3,t0,t1,t2,t3,t4,t5)) ;
}

void
transformOP(void)
{
	double		x, y ;
	struct	object	object, junk ;
	struct	pspoint	ppoint;

	object = pop() ;
	switch(object.type){
	case OB_ARRAY :
		object = realmatrix(object) ;
		junk=real("transform");
		y = junk.value.v_real ;
		junk=real("transform");
		x = junk.value.v_real ;
		push(makereal(E(object,0)*x+E(object,2)*y+E(object,4))) ;
		push(makereal(E(object,1)*x+E(object,3)*y+E(object,5))) ;
		return;

	case OB_INT :
		push(object) ;
		object = real("transform") ;
	case OB_REAL :
		junk=real("transform");
		ppoint = _transform(makepoint(junk.value.v_real,object.value.v_real)) ;
		pushpoint(ppoint) ;
		return;

	default :
		fprintf(stderr,"transform %d\n",object.type);
		pserror("typecheck", "transform") ;
	}
}

struct	pspoint
_transform(struct pspoint ppoint)
{
	return(makepoint(G[0]*ppoint.x+G[2]*ppoint.y+G[4],G[1]*ppoint.x+G[3]*ppoint.y+G[5])) ;
}

struct	pspoint
dtransform(struct pspoint p)
{
	struct	pspoint	ppoint ;

	ppoint.x = G[0]*p.x+G[2]*p.y ;
	ppoint.y = G[1]*p.x+G[3]*p.y ;
	return(ppoint) ;
}

void
dtransformOP(void)
{
	double		x, y ;
	struct	object	object, junk ;
	struct	pspoint	ppoint ;

	object = pop() ;
	switch(object.type){
	case OB_ARRAY :
		object = realmatrix(object) ;
		junk=real("dtransform");
		y = junk.value.v_real ;
		junk=real("dtransform");
		x = junk.value.v_real ;
		push(makereal(E(object,0)*x+E(object,2)*y)) ;
		push(makereal(E(object,1)*x+E(object,3)*y)) ;
		return;

	case OB_INT :
		push(object) ;
		object = real("dtransform") ;
	case OB_REAL :
		junk=real("dtransform");
		ppoint = dtransform(makepoint(junk.value.v_real,
			object.value.v_real)) ;
		pushpoint(ppoint) ;
		return;

	default :
		fprintf(stderr,"dtransform %d\n",object.type);
		pserror("typecheck", "dtransform") ;
	}
}

void
initmatrixOP(void)
{
	int	i ;
	for ( i=0 ; i<CTM_SIZE ; i++ )
		Graphics.CTM[i] = Graphics.device.matrix[i] ;
	fontchanged = 1;
}

void
defaultmatrixOP(void)
{

	push(makematrix(pop(),
		Graphics.device.matrix[0],
		Graphics.device.matrix[1],
		Graphics.device.matrix[2],
		Graphics.device.matrix[3],
		Graphics.device.matrix[4],
		Graphics.device.matrix[5])) ;
}

void
invertmatrixOP(void)
{
	double		d ;
	struct	object	inverse ;
	struct	object	matrix ;

	inverse = pop() ;
	matrix = realmatrix(pop()) ;
	d = E(matrix,0)*E(matrix,3) - E(matrix,1)*E(matrix,2) ;
	if ( d == 0.0 )
		pserror("undefinedresult", "invertmatrix") ;
	push(makematrix(inverse,
		E(matrix,3) / d,
		-E(matrix,1) / d,
		-E(matrix,2) / d,
		E(matrix,0) / d,
		(E(matrix,5)*E(matrix,2) - E(matrix,4)*E(matrix,3)) / d,
		(E(matrix,4)*E(matrix,1) - E(matrix,5)*E(matrix,0)) / d)) ;
}

struct	pspoint
itransform(struct pspoint p)
{
	double		d ;
	struct	pspoint	ppoint ;

	d = G[0]*G[3] - G[1]*G[2] ;
	if ( d == 0.0 )
		pserror("undefinedresult", "itransform") ;
	ppoint.x = (G[3]*p.x-G[2]*p.y+(G[5]*G[2] - G[4]*G[3])) / d ;
	ppoint.y = (-G[1]*p.x+G[0]*p.y+(G[4]*G[1] - G[5]*G[0])) / d ;
	return(ppoint) ;
}

struct	pspoint
idtransform(struct pspoint p)
{
	double		d ;
	struct	pspoint	ppoint ;

	d = G[0]*G[3] - G[1]*G[2] ;
	if ( d == 0.0 ){
		return(makepoint(0.,0.));	/*seems to be what the printer does internally*/
		/*pserror("undefinedresult - idtransform") ;	*/
	}
	ppoint.x = (G[3]*p.x-G[2]*p.y) / d ;
	ppoint.y = (-G[1]*p.x+G[0]*p.y) / d ;
	return(ppoint) ;
}

void
itransformOP(void)
{
	struct	object	object ;

	object = pop() ;
	switch(object.type){
	case OB_ARRAY :
		push(object) ;
		matrixOP() ;
		invertmatrixOP() ;
		transformOP() ;
		return;

	case OB_INT :
	case OB_REAL :
		push(object) ;
		matrixOP() ;
		currentmatrixOP() ;
		matrixOP() ;
		invertmatrixOP() ;
		transformOP() ;
		return;

	default :
		pserror("typecheck", "itransform") ;
	}
}

void
idtransformOP(void)
{
	struct	object	object ;

	object = pop() ;
	switch(object.type){
	case OB_ARRAY :
		push(object) ;
		matrixOP() ;
		invertmatrixOP() ;
		dtransformOP() ;
		return;

	case OB_INT :
	case OB_REAL :
		push(object) ;
		matrixOP() ;
		currentmatrixOP() ;
		matrixOP() ;
		invertmatrixOP() ;
		dtransformOP() ;
		return;

	default :
		pserror("typecheck", "idtransform") ;
	}
}

struct	object
realmatrix(struct object matrix)
{
	int	i ;

	if ( matrix.type != OB_ARRAY || matrix.value.v_array.length != CTM_SIZE ){
		fprintf(stderr,"mtype %d size %d\n",matrix.type,
			matrix.value.v_array.length);
		pserror("typecheck", "realmatrix") ;
	}
	for ( i=0 ; i<CTM_SIZE ; i++ )
		switch(matrix.value.v_array.object[i].type){
		case OB_INT :
			SAVEITEM(matrix.value.v_array.object[i]) ;
			matrix.value.v_array.object[i].type = OB_REAL ;
			matrix.value.v_array.object[i].value.v_real =
			  (double)matrix.value.v_array.object[i].value.v_int ;
			break ;

		case OB_REAL :
			break ;

		default :
			pserror("typecheck", "realmatrix") ;
		}
	return(matrix) ;
}
struct	object
makematrix(struct object matrix, double r0, double r1, double r2, double r3,
	double r4, double r5)
{

	if ( matrix.type != OB_ARRAY  ||  matrix.value.v_array.length != CTM_SIZE){
		fprintf(stderr,"type %d val %d",matrix.type, matrix.value.v_int);
		pserror("typecheck", "makematrix") ;
	}
	saveitem(matrix.value.v_array.object,
		sizeof(matrix.value.v_array.object[0])*CTM_SIZE) ;
	matrix.value.v_array.object[0] = makereal(r0) ;
	matrix.value.v_array.object[1] = makereal(r1) ;
	matrix.value.v_array.object[2] = makereal(r2) ;
	matrix.value.v_array.object[3] = makereal(r3) ;
	matrix.value.v_array.object[4] = makereal(r4) ;
	matrix.value.v_array.object[5] = makereal(r5) ;
	return(matrix) ;
}
