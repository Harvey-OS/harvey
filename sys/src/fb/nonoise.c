/*% cyntax % -lfb && cc -go # % -lfb
 * Bayer-Powell noise removal filter.
 * If the average of the neighbors differs from
 * the center by more than 64, replace the center
 * with the average of the nieghbors.
 */
#include "filter.cc"
void filter(unsigned char *l0, unsigned char *l1, unsigned char *l2, int nchan, int npix){
	unsigned char *op=outline;
	int periph, eps;
	npix*=nchan;
	if(npix==0) return;
	do{
		periph=l0[-nchan]+l0[0]+l0[nchan]
		      +l1[-nchan]      +l1[nchan]
		      +l2[-nchan]+l2[0]+l2[nchan];
		eps=periph-8*l1[0];
		if(eps<0) eps=-eps;
		*op++=eps>64*8?periph>>3:l1[0];
		l0++;
		l1++;
		l2++;
	}while(--npix!=0);
}
