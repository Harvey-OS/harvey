/*
 * Extremum filter -- replace each pixel by the 8-neighbor that
 * differs most from it
 */
#include "filter.cc"
void filter(unsigned char *l0, unsigned char *l1, unsigned char *l2, int nchan, int npix){
	int min, max;
	unsigned char *op=outline;
	npix*=nchan;
	if(npix==0) return;
	do{
		min=max=l0[-nchan];
#define minmax(a) if(a<min) min=a; else if(max<a) max=a
		minmax(l0[0]);
		minmax(l0[nchan]);
		minmax(l1[-nchan]);
		minmax(l1[0]);
		minmax(l1[nchan]);
		minmax(l2[-nchan]);
		minmax(l2[0]);
		minmax(l2[nchan]);
		*op++=max-l0[0]<l0[0]-min?max:min;
		l0++;
		l1++;
		l2++;
	}while(--npix!=0);
}
