/*
 * Posix stdio -- fdopen
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
FILE *fdopen(const int fd, const char *mode){
	FILE *f;	
	qlock(&_stdiolk);
	for(f=_IO_stream;f!=&_IO_stream[FOPEN_MAX];f++)
		if(f->state==CLOSED)
			break;
	if(f==&_IO_stream[FOPEN_MAX]) {
		qunlock(&_stdiolk);
		return NULL;
	}
	f->fd=fd;
	if(mode[0]=='a')
		seek(f->fd, 0L, 2);
	if(f->fd==-1) return NULL;
	f->flags=0;
	f->state=OPEN;
	f->buf=0;
	f->rp=0;
	f->wp=0;
	f->lp=0;
	qunlock(&_stdiolk);
	return f;
}
