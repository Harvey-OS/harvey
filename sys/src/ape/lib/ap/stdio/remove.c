/*
 * pANS stdio -- remove
 */
#include "iolib.h"
int remove(const char *f){
	return unlink((char *)f);
}
