/*
 * pANS stdio -- clearerr
 */
#include "iolib.h"
void clearerr(FILE *f){
	switch(f->state){
	case ERR:
		f->state=f->buf?RDWR:OPEN;
		break;
	case END:
		f->state=RDWR;
		break;
	}
}
