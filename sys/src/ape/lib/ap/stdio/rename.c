/*
 * pANS stdio -- rename
 */
#include "iolib.h"
int rename(const char *old, const char *new){
	if(link((char *)old, (char *)new)<0) return -1;
	if(unlink((char *)old)<0){
		unlink((char *)new);
		return -1;
	}
	return 0;
}
