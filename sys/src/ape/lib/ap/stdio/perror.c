/*
 * pANS stdio -- perror
 */
#include "iolib.h"
void perror(const char *s){
	extern int errno;
	if(s!=NULL && *s != '\0') fputs(s, stderr), fputs(": ", stderr);
	fputs(strerror(errno), stderr);
	putc('\n', stderr);
	fflush(stderr);
}
