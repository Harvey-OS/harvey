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
/* premature; fall through and see what happens */
/*		f->state=OPEN; */
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
		f->fd=open(name, (*mode == '+'? O_RDWR: O_RDONLY));
		break;
	case 'w':
		f->fd=creat(name, 0666);	/* implicitly O_WRONLY */
		/* for O_RDWR, have to creat, close, open */
		if(*mode == '+' && f->fd >= 0) {
			close(f->fd);
			f->fd=open(name, O_RDWR);
		}
		break;
	case 'a':
		f->fd=open(name, (*mode == '+'? O_RDWR: O_WRONLY));
		if(f->fd<0) {
			f->fd=creat(name, 0666);
			/* for O_RDWR, have to creat, close, open */
			if(*mode == '+' && f->fd >= 0) {
				close(f->fd);
				f->fd=open(name, O_RDWR);
			}
		}
		lseek(f->fd, 0L, 2);
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
