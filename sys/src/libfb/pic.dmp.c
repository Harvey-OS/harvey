/*
 * Type-specific code for TYPE=dump
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
/*
 * Read exactly the given number of bytes from fd into buf.
 * Returns 1 if successful, 0 if eof occurs before buf is full, -1 on error.
 */
int _PICread(int fd, void *buf, int need){
	int got;
	for(;need!=0;need-=got,buf=(char *)buf+got){
		got=read(fd, buf, need);
		if(got<=0) return got;
	}
	return 1;
}
int _PRDdmp(PICFILE *f, void *buf){
	f->line++;
	return _PICread(f->fd, buf, f->width*f->nchan)>0;
}
int _PWRdmp(PICFILE *f, void *buf){
	if(f->line==0) _PWRheader(f);
	f->line++;
	return write(f->fd, buf, f->width*f->nchan)==f->width*f->nchan;
}
void _PCLdmp(PICFILE *f){
	USED(f);
}
