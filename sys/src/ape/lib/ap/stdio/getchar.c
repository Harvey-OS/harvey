/*
 * pANS stdio -- getchar
 */
#include "iolib.h"
#undef getchar
int getchar(void){
	return fgetc(stdin);
}
