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
setgrayOP(void)
{
	int	gray, gin ;
	double fr;
	
	gray = floor((fr=fraction())*(GRAYLEVELS-1)+0.5) ;
fprintf(stderr,"setgray %f\n",fr);
	Graphics.color.red = gray ;
	Graphics.color.green = gray ;
	Graphics.color.blue = gray ;
}

void
sethsbcolorOP(void)
{
	double		h, s, b;

	b = fraction() ;
	s = fraction() ;
	h = fraction() ;
	Graphics.color = hsb2rgb(h,s,b) ;
}

void
setrgbcolorOP(void)
{
	Graphics.color.blue = floor(fraction()*(GRAYLEVELS-1)+0.5) ;
	Graphics.color.green = floor(fraction()*(GRAYLEVELS-1)+0.5) ;
	Graphics.color.red = floor(fraction()*(GRAYLEVELS-1)+0.5) ;
}

void
currentgrayOP(void)
{
	push(makereal(currentgray())) ;
}

double
currentgray(void)
{
	double gy;
	gy= 0.3 * Graphics.color.red/(double)(GRAYLEVELS-1) +
		0.59 * Graphics.color.green/(double)(GRAYLEVELS-1) +
		0.11 * Graphics.color.blue/(double)(GRAYLEVELS-1) ;
/*	if(gy > .9)gy=.9;*/
	return(gy);
}

void
currenthsbcolorOP(void)
{
	double		red, green, blue, hue, saturation, brightness;
	double		x, r, g, b, d;

	red = Graphics.color.red/(double)(GRAYLEVELS-1) ;
	green = Graphics.color.green/(double)(GRAYLEVELS-1) ;
	blue = Graphics.color.blue/(double)(GRAYLEVELS-1) ;

	brightness = red ;
	if ( green > brightness )
		brightness = green ;
	if ( blue > brightness )
		brightness = blue ;

	x = red ;
	if ( green < x )
		x = green ;
	if ( blue < x )
		x = blue ;

	d = brightness - x ;
	if ( d == 0 )
		saturation = 0 ;
	else{
		saturation = d / brightness ;
		r = (brightness - red) / d ;
		g = (brightness - green) / d ;
		b = (brightness - blue) / d ;
		if ( red == brightness )
			if ( green == x )
				hue = 5 + b ;
			else
				hue = 1 - g ;
		if ( green == brightness )
			if ( blue == x )
				hue = 1 + r ;
			else
				hue = 3 - b ;
		if ( blue == brightness )
			if ( red == x )
				hue = 3 + g ;
			else
				hue = 5 - r ;
		hue /= 6 ;
	}
	push(makereal(hue)) ;
	push(makereal(saturation)) ;
	push(makereal(brightness)) ;
}

void
currentrgbcolorOP(void)
{

	push(makereal(Graphics.color.red/(double)(GRAYLEVELS-1))) ;
	push(makereal(Graphics.color.green/(double)(GRAYLEVELS-1))) ;
	push(makereal(Graphics.color.blue/(double)(GRAYLEVELS-1))) ;
}

double
fraction(void)
{
	double		value ;
	struct	object	object ;

	object = real("fraction");
	if(object.type == OB_NULL)return(0.);
	value = object.value.v_real ;
	if(value < 0.0)return(0.0);
	if(value > 1.0)return(1.0);
	return(value) ;
}

struct	color
hsb2rgb(double h, double s, double b)
{
	struct	color	color ;
	int		i ;
	double		red, green, blue, f, k, m, n;

	h *= 6 ;
	i = floor(h) ;
	f = h - i ;
	m = b * (1-s) ;
	n = b * (1-s*f) ;
	k = b * (1-s*(1-f)) ;
	switch(i){
	case 0 :
	case 6 :
		red = b ;
		green = k ;
		blue = m ;
		break ;

	case 1 :
		red = n ;
		green = b ;
		blue = m ;
		break ;

	case 2 :
		red = m ;
		green = b ;
		blue = k ;
		break ;

	case 3 :
		red = m ;
		green = n ;
		blue = b ;
		break ;

	case 4 :
		red = k ;
		green = m ;
		blue = b ;
		break ;

	case 5 :
		red = b ;
		green = m ;
		blue = n ;
		break ;

	default :
		fprintf(stderr, "i=%d\n",i);
		error("bad i in hsb2rgb") ;
	}
	color.red = floor(red*(GRAYLEVELS-1)+0.5) ;
	color.green = floor(green*(GRAYLEVELS-1)+0.5) ;
	color.blue = floor(blue*(GRAYLEVELS-1)+0.5) ;

	return(color) ;
}
static int junk=1;
void
settransferOP(void)
{
	int		i ;
if(!junk){
	junk++;
	Graphics.color.red = 230 ;
	Graphics.color.green = 230;
	Graphics.color.blue = 230 ;
}
	
fprintf(stderr,"settransfer called\n");
pstackOP();
fprintf(stderr,"done stack\n");
	Graphics.transfer = procedure() ;
	for ( i=0 ; i<GRAYLEVELS ; i++ ){
		push(makereal(i/(double)(GRAYLEVELS-1))) ;
		execpush(Graphics.transfer) ;
		execute() ;
		Graphics.graytab[i] = fraction() ;
/*fprintf(stderr,"was %f new %f\n",i/(double)(GRAYLEVELS-1),Graphics.graytab[i]);*/
	}
}

void
currenttransferOP(void)
{
	push(Graphics.transfer) ;
}
