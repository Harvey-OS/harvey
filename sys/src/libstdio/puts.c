/*
 * pANS stdio -- puts
 */
#include "iolib.h"
int puts(const char *s){
	fputs(s, stdout);
	putchar('\n');
	return ferror(stdin)?EOF:0;
}
