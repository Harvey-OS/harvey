/*
 * Type-specific code for TYPE=pico
 * Not a standard type, but supported for Gerard.
 * Doesn't work on pipes because it calls seek.
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
int _PRDpco(PICFILE *f, void *vbuf){
	unsigned char *buf=vbuf, *bufp, *ein, *inp;
	int i, n;
	long offs;
	if(f->line==0){
		f->buf=malloc(f->width);
		if(f->buf==0){
			_PICerror="Can't allocate buffer";
			return 0;
		}
	}
	if(f->line==f->height){
		_PICerror="Read past end of picture";
		return 0;
	}
	offs=seek(f->fd, 0L, 1);
	ein=f->buf+f->width;
	for(i=0;i!=f->nchan;i++){
		n=_PICread(f->fd, f->buf, f->width);
		if(n<=0){
			if(n==0) _PICerror="End of file reading picture";
			return 0;
		}
		inp=f->buf;
		for(bufp=buf+i;inp!=ein;bufp+=f->nchan)
			*bufp=*inp++;
		seek(f->fd, (long)f->width*(f->height-1), 1);
	}
	seek(f->fd, offs+f->width, 0);
	f->line++;
	return 1;
}
int _PWRpco(PICFILE *f, void *vbuf){
	unsigned char *buf=vbuf, *bufp, *eout, *outp;
	int i, n;
	long offs;
	if(f->line==0){
		f->buf=malloc(f->width);
		if(f->buf==0){
			_PICerror="Can't allocate buffer";
			return 0;
		}
		_PWRheader(f);
	}
	if(f->line==f->height){
		_PICerror="Write past end of picture";
		return 0;
	}
	offs=seek(f->fd, 0L, 1);
	eout=f->buf+f->width;
	for(i=0;i!=f->nchan;i++){
		outp=f->buf;
		for(bufp=buf+i;outp!=eout;bufp+=f->nchan)
			*outp++=*bufp;
		n=write(f->fd, f->buf, f->width);
		if(n!=f->width) return 0;
		seek(f->fd, (long)f->width*(f->height-1), 1);
	}
	seek(f->fd, offs+f->width, 0);
	f->line++;
	return 1;
}
void _PCLpco(PICFILE *f){
	if(f->buf) free(f->buf);
}
