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
	if(f->state!=CLOSED) fclose(f);
	if(mode[1]=='+' || mode[2]=='+'){
		if(mode[0]=='w') f->fd=create(name, 2, 0666);
		else{
			f->fd=open(name, 2);
			if(mode[0]=='a'){
				if(f->fd<0)
					f->fd=create(name, 2, 0666);
				if(f->fd>=0)
					seek(f->fd, 0L, 2);
			}
		}
	}
	else{
		switch(mode[0]){
		default: return NULL;
		case 'r':
			f->fd=open(name, 0);
			break;
		case 'w':
			f->fd=create(name, 1, 0666);
			break;
		case 'a':
			f->fd=open(name, 1);
			if(f->fd==-1)
				f->fd=create(name, 1, 0666);
			seek(f->fd, 0L, 2);
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
