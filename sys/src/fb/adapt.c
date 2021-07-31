/*
 * adaptive contrast enhancement -- a 7 by 7 neighbourhood around each pixel
 * is examined  for its minimum and maximum values.  The center pixel is
 * remapped linearly in a way that would send the neighbourhood's maximum to
 * 255 and its minimum to 0.  cen=255*(cen-min)/(max-min).
 */
#define	NWIN	7	/* size of window */
#include "map.cc"
int filter(unsigned char *b){
	register unsigned char min, max;
	register unsigned char *eb=b+NWIN*NWIN;
	register unsigned char cen=b[NWIN/2*(NWIN+1)];
	for(min=max=*b++;b!=eb;b++) if(*b<min) min=*b; else if(max<*b) max=*b;
	if(min==max) return cen;
	return 255*(cen-min)/(max-min);
}
