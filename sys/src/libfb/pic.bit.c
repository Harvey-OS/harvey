/*
 * Type-specific code for TYPE=bitmap
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
int _PRDbit(PICFILE *f, void *vbuf){
	unsigned char *buf=vbuf, *bufp, *ebuf, *bitp;
	int bit;
	int n, bwid=(f->width+15)/16*2;
	if(f->line==0){
		f->buf=malloc(bwid);
		if(f->buf==0){
			_PICerror="Can't allocate buffer";
			return 0;
		}
	}
	if(f->line==f->height){
		_PICerror="Read past end of picture";
		return 0;
	}
	bitp=f->buf;
	n=_PICread(f->fd, bitp, bwid);
	if(n<=0){
		if(n==0) _PICerror="End of file reading picture";
		return 0;
	}
	ebuf=buf+f->width;
	bit=128;
	for(bufp=buf;bufp!=ebuf;){
		*bufp++=*bitp&bit?0:255;
		bit>>=1;
		if(bit==0){
			bitp++;
			bit=128;
		}
	}
	f->line++;
	return 1;
}
int _PWRbit(PICFILE *f, void *vbuf){
	unsigned char *buf=vbuf, *bufp, *ebuf, *bitp;
	int bit, byte;
	int n, bwid=(f->width+15)/16*2;
	if(f->line==0){
		f->buf=malloc(bwid);
		f->buf[bwid-1]=0;	/* might be a slack byte */
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
	bitp=f->buf;
	ebuf=buf+f->width;
	byte=0;
	bit=128;
	for(bufp=buf;bufp!=ebuf;){
		if(*bufp++<128)
			byte|=bit;
		bit>>=1;
		if(bit==0){
			*bitp++=byte;
			bit=128;
			byte=0;
		}
	}
	if(bit!=128) *bitp=byte;
	f->line++;
	n=write(f->fd, f->buf, bwid);
	if(n!=bwid) return 0;
	return 1;
}
void _PCLbit(PICFILE *f){
	if(f->buf) free(f->buf);
}
