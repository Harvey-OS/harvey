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
FILE *freopen(const char *nm, const char *mode, FILE *f){
	if(f->state!=CLOSED) fclose(f);
	if(mode[1]=='+' || mode[2]=='+'){
		if(mode[0]=='w') close(creat(nm, 0666));
		f->fd=open(nm, O_RDWR);
		if(f->fd==-1){
			close(creat(nm, 0666));
			f->fd=open(nm, O_RDWR);
		}
		if(mode[0]=='a') lseek(f->fd, 0L, 2);
	}
	else{
		switch(mode[0]){
		default: return NULL;
		case 'r':
			f->fd=open(nm, O_RDONLY);
			break;
		case 'w':
			f->fd=creat(nm, 0666);
			break;
		case 'a':
			f->fd=open(nm, O_WRONLY);
			if(f->fd==-1)
				f->fd=creat(nm, 0666);
			lseek(f->fd, 0L, 2);
			break;
		}
	}
	if(f->fd==-1) return NULL;
	f->flags=(mode[0]=='a')? APPEND : 0;
	f->state=OPEN;
	f->buf=0;
	f->rp=0;
	f->wp=0;
	f->lp=0;
	return f;
}
