/*
 * pANS stdio -- vprintf
 */
#include "iolib.h"
int vprintf(const char *fmt, va_list args){
	return vfprintf(stdout, fmt, args);
}
