/*% cyntax % -lfb && cc -go # % -lfb
 * Laplacian, kernel:
 *  0 -1  0
 * -1  5 -1
 *  0 -1  0
 */
#include "filter.cc"
void filter(unsigned char *l0, unsigned char *l1, unsigned char *l2, int nchan, int npix){
	unsigned char *op=outline;
	int v;
	npix*=nchan;
	if(npix==0) return;
	do{
		v=5*l1[0]-l0++[0]-l1[-nchan]-l1[nchan]-l2++[0];
		l1++;
		*op++=v<0?0:v<=255?v:255;
	}while(--npix!=0);
}
