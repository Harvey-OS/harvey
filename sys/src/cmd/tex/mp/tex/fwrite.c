/*
 * pANS stdio -- fwrite
 */
#include "iolib.h"
size_t fwrite(const void *p, size_t recl, size_t nrec, FILE *f){
	char *s=(char *)p, *es=s+recl*nrec;
	while(s!=es && putc(*s, f)!=EOF) s++;
	return (s-(char *)p)/(recl?recl:1);
}
