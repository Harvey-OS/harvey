#include <u.h>
#include <libc.h>
#include "system.h"
#ifdef Plan9
#include <libg.h>
#endif

# include "stdio.h"
# include "defines.h"
# include "object.h"
# include "path.h"
#include "njerq.h"
# include "graphics.h"
#ifdef VAX
unsigned char bytemap[256];
#endif

void
init(char *cacheflg)
{

	Line_no = 1 ;
	savestackinit() ;
	operstackinit() ;
	mybinit();		/* must be after saveinit & before dictinit*/
	dictstackinit() ;
	execstackinit() ;
	graphicsstackinit() ;
	pathinit() ;
	deviceinit() ;
	initgraphicsOP() ;
	type1init() ;

	Graphics.flat = FLAT ;
	push(makearray(0,XA_EXECUTABLE)) ;
	settransferOP();
	Graphics.screen.frequency = 20.0 ;
	Graphics.screen.angle = 0.0 ;
	Graphics.screen.proc = makearray(1,XA_EXECUTABLE) ;
	Graphics.screen.proc.value.v_array.object[0] = makeoperator(popOP) ;
}
#ifdef Plan9
Bitmap	*pgrey[GRAYIMP], *ones;
unsigned char grey[GRAYIMP*32];
unsigned char grey_proto[GRAYIMP][8] = {
	0xff, 0xff, 0xff, 0xff,0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,0xff, 0xff, 0xbb, 0xbb,
	0xdd, 0xdd, 0xff, 0xff,0xff, 0xff, 0xbb, 0xbb,
	0xdd, 0xdd, 0xff, 0xff,0xff, 0xff, 0x99, 0x99,

	0x99, 0x99, 0xff, 0xff,0xff, 0xff, 0x99, 0x99,
	0x99, 0x99, 0xee, 0xee,0xff, 0xff, 0x99, 0x99,
	0x99, 0x99, 0xee, 0xee,0x77, 0x77, 0x99, 0x99,
	0x99, 0x99, 0xee, 0xee,0x66, 0x66, 0x99, 0x99,

	0x99, 0x99, 0x66, 0x66,0x66, 0x66, 0x99, 0x99,
	0x88, 0x88, 0x66, 0x66,0x66, 0x66, 0x99, 0x99,
	0x88, 0x88, 0x66, 0x66,0x66, 0x66, 0x11, 0x11,
	0x88, 0x88, 0x66, 0x66,0x66, 0x66, 0x00, 0x00,

	0x00, 0x00, 0x66, 0x66,0x66, 0x66, 0x00, 0x00,
	0x00, 0x00, 0x44, 0x44,0x66, 0x66, 0x00, 0x00,
	0x00, 0x00, 0x44, 0x44,0x22, 0x22, 0x00, 0x00,
	0x00, 0x00, 0x44, 0x44,0x00, 0x00, 0x00, 0x00,

	0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0,
};
#endif
#ifdef FAX
Rectangle Drect = { 0, 0, FXMAX, YMAX };
#else
Rectangle Drect = { 0, 0, XMAX, YMAX };
#endif
Bitmap *bp;
void
mybinit(void)
{
	int i, j, k=0, gi;
	unsigned char *g;
	Rectangle r;

	for( gi = 0; gi < GRAYIMP; gi++ ) {
		for( i = 0; i < 4; i++ ) {
			for( j = 0; j < 8; j++ ){
				grey[k++] = grey_proto[gi][j];
			}
		}
	}
#ifdef VAX
	for (i = 128, j = 1; i > 0; i >>= 1, j <<= 1)
		for (k = 0; k < 256; k++)
			if (k&i)
				bytemap[k] |= j;
#endif
#ifndef Plan9
	bp = balloc(Drect, 0);
	if(bp == (Bitmap *)NULL){
		fprintf(stderr,"failed to init - space\n");
		done(1);
	}
	screen = *bp;
	for(i=0;i<GRAYIMP;i++)
		pgrey[i] = &grey[i];
#else
	binit(0, 0, "psi");
	einit(Emouse|Ekeyboard);
	bp = &screen;
	if(bp == (Bitmap *)NULL){
		fprintf(stderr,"failed in init - space\n");
		done(1);
	}
	g =(unsigned char *) grey;
	for(i=0; i<17; i++){
		pgrey[i] = balloc(Rect(0, 0, 16, 16), 0);
		wrbitmap(pgrey[i], 0, 16, g);
		g += 32;
	}
	ones = pgrey[0];
	r = inset(screen.r, 6);
	orig = screen.r.min;
	corn = screen.r.max;
	texture(&screen, r, pgrey[16], Zero);
#endif
}
