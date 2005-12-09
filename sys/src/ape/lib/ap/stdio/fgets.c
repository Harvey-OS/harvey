/*
 * pANS stdio -- fgets
 */
#include "iolib.h"
char *fgets(char *as, int n, FILE *f){
	int c=0;
	char *s=as;
	while(n>1 && (c=getc(f))!=EOF){
		*s++=c;
		--n;
		if(c=='\n') break;
	}
	if(c==EOF && s==as
	|| ferror(f)) return NULL;
	if(n) *s='\0';
	return as;
}
