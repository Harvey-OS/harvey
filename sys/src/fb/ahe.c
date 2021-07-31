/*
 * adaptive histogram equalization over 17 by 17 squares
 */
#define	NWIN	17	/* size of window */
#include "map.cc"
filter(unsigned char *b){
	register unsigned char *eb;
	register unsigned char cen=b[NWIN/2*(NWIN+1)];
	register nlo=0;
	for(eb=b+NWIN*NWIN;b!=eb;b++)
		if(*b==cen) nlo++;
		else if(*b<cen) nlo+=2;
	return 255*nlo/(2*NWIN*NWIN);
}
