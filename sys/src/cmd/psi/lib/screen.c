 # include <u.h>
#include <libc.h>
#include "system.h"
#ifdef Plan9
#include <libg.h>
#endif

# include "stdio.h"
# include "njerq.h"
# include "defines.h"
# include "object.h"
# include "path.h"
# include "graphics.h"

# define RESOLUTION	300
# define MAXLEVELS	300

void
setscreenOP(void)
{
	int		i, j, k, levels, cycles, cyclen, smsquares, x, y, dx, dy ;
	double		frequency, angle, period, fx, fy ;
	struct	hta	hta[MAXLEVELS] ;
	struct	object	proc, object;	

/* watch out for dx & dy being <0  */
fprintf(stderr,"setscreen stack\n");
pstackOP();
	proc = procedure() ;
	object = real("screen");
	angle = object.value.v_real ;
	object = real("screen");
	frequency = object.value.v_real ;
	Graphics.screen.frequency = frequency ;
	Graphics.screen.angle = angle ;
	Graphics.screen.proc = proc ;
fprintf(stderr,"setscreen called freq %f %f\n", frequency, angle);

	period = RESOLUTION / frequency ;
	dx = floor(period * cos(angle*M_PI/180) + 0.5) ;
	dy = floor(period * sin(angle*M_PI/180) + 0.5) ;
	levels = dx*dx + dy*dy ;
	fprintf(stderr,"dx=%d   dy=%d lev %d\n",dx,dy,levels) ;
	if ( levels > MAXLEVELS ){
		fprintf(stderr,"levels %d MAX %d\n",levels,MAXLEVELS);
		pserror("limitcheck", "setscreen") ;
	}
	x = dy - dx ;
	y = -dx ;

	cycles = gcd(abs(dx),abs(dy)) ;
	cyclen = levels / cycles ;	
	smsquares = cyclen / cycles ;
fprintf(stderr,"cycles %d cyclen %d\n",cycles, cyclen);
	if(1)return;

	k = 0 ;
	for ( i=0 ; i<cycles ; i++ ){
		x += dx ;
		y += dy ;
		for ( j=0 ; j<cyclen ; j++ ){
			x -= dy ;
			y += dx ;
			fx = (double)rem(x,levels)/levels*2.0-1.0+(dx-dy)/(double)levels ;
			fy = (double)rem(y,levels)/levels*2.0-1.0+(dx-dy)/(double)levels ;

/*			printf("%lf\t%lf\n",fx,fy) ; */
			push(makereal(fx)) ;
			push(makereal(fy)) ;
			execpush(proc) ;
			execute() ;
			object = real("screen");
			hta[k].order = object.value.v_real ;
			if ( hta[k].order < -1  ||  hta[k].order > 1 )
				pserror("rangecheck", "setscreen") ;
			hta[k].x = floor((dx*fx + dy*fy)/2+0.25) ;
			hta[k].y = floor((dx*fy - dy*fx)/2+0.25) ;
/*			printf("(%lf\t%lf\n",(dx*fx + dy*fy)/2+0.25,(dx*fy - dy*fx)/2+0.25) ;*/
/*			printf("(%d,%d)\n",hta[k].x,hta[k].y) ;*/
			k++ ;
		}
	}
	qsort(hta,levels,sizeof(hta[0]),htcomp) ;
	

	Graphics.screen.bitmap[0] = balloc(Rect(0, 0,cyclen,cyclen),0) ;
	for ( k=1 ; k<levels ; k++ ){
		Graphics.screen.bitmap[k] = balloc(Rect(0, 0,cyclen,cyclen),0) ;
		bitblt(Graphics.screen.bitmap[k],
			Graphics.screen.bitmap[k]->r.min, 
			Graphics.screen.bitmap[k-1],
			Graphics.screen.bitmap[k-1]->r, S);
		x = hta[k].x ;
		y = hta[k].y ;
		for ( i=0 ; i<smsquares ; i++ ){
			point(Graphics.screen.bitmap[k],Pt(rem(x,cyclen),rem(y,cyclen)),~0,S) ;
			x += dx ;
			y += dy ;
		}

/*		printf("---\n") ;
		for ( i=0 ; i<Graphics.screen.bitmap[k].height ; i++ ){
			for ( j=0 ; j<Graphics.screen.bitmap[k]->width ; j++ )
				putchar(bmget(Graphics.screen.bitmap[k],j,i)==1?'*':' ') ;
			putchar('\n') ;
		}
*/
	}
}

int
htcomp(struct hta *a, struct hta *b)
{
	if ( a->order < b->order )
		return(-1) ;
	if ( a->order > b->order )
		return(1) ;
	return(0) ;
}

int
gcd(int m, int n)
{
	int	t ;

	if ( m > n ){
		t = m ;
		m = n ;
		n = t ;
	}
	while( m > 0 ){
		t = n - m ;
		if ( t > m )
			n = t ;
		else{
			n = m ;
			m = t ;
		}
	}
	return(n) ;
}

int
rem(int n, int mod)
{
	while ( n >= 0 )
		n -= mod ;
	while ( n < 0 )
		n += mod ;
	return(n) ;
}

void
currentscreenOP(void)
{
	push(makereal(Graphics.screen.frequency)) ;
	push(makereal(Graphics.screen.angle)) ;
	push(Graphics.screen.proc) ;
}
