/*
 * smooth -- Bartlett window low-pass filter with kernel
 * 1 2 1
 * 2 4 2
 * 1 2 1
 */
#include "filter.cc"
void filter(unsigned char *l0, unsigned char *l1, unsigned char *l2, int nchan, int npix){
	unsigned char *op=outline;
	npix*=nchan;
	if(npix==0) return;
	do{
		*op++=(   l0[-nchan]+2*l0[0]+  l0[nchan]
		       +2*l1[-nchan]+4*l1[0]+2*l1[nchan]
		       +  l2[-nchan]+2*l2[0]+  l2[nchan])/16;
		l0++;
		l1++;
		l2++;
	}while(--npix!=0);
}
