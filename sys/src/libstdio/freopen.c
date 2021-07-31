/*
 * pANS stdio -- freopen
 */
#include "iolib.h"
/*
 * Open the named file with the given mode, using the given FILE
 * Legal modes are given below, `additional characters may follow these sequences':
 * r rb		open to read
 * w wb		open to write, truncating
 * a ab		open to write positioned at eof, creating if non-existant
 * r+ r+b rb+	open to read and write, creating if non-existant
 * w+ w+b wb+	open to read and write, truncating
 * a+ a+b ab+	open to read and write, positioned at eof, creating if non-existant.
 */
FILE *freopen(const char *name, const char *mode, FILE *f){
	int m;

	if(f->state!=CLOSED){
		fclose(f);
		f->state=OPEN;
	}

	m = *mode++;
	if(m == 0)
		return NULL;
	if(*mode == 'b')
		mode++;
	switch(m){
	default:
		return NULL;
	case 'r':
		m = 0;
		if(*mode == '+') m = 2;
		f->fd=open(name, m);
		break;
	case 'w':
		m = 1;
		if(*mode == '+') m = 2;
		f->fd=create(name, m, 0666);
		break;
	case 'a':
		m = 1;
		if(*mode == '+') m = 2;
		f->fd=open(name, m);
		if(f->fd<0)
			f->fd=create(name, m, 0666);
		seek(f->fd, 0L, 2);
		break;
	}

	if(f->fd==-1)
		return NULL;
	f->flags=(mode[0]=='a')? APPEND : 0;
	f->state=OPEN;
	f->buf=0;
	f->rp=0;
	f->wp=0;
	f->lp=0;
	return f;
}
