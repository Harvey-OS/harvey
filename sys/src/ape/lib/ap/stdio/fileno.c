/*
 * Posix stdio -- fileno
 */
#include "iolib.h"
int fileno(FILE *f){
	if(f==NULL)
		return -1;
	else
		return f->fd;
}
