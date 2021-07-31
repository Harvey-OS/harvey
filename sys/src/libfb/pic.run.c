/*
 * Type-specific code for TYPE=runcode
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#define	NBUF	4096	/* size of input buffer */
int _PRDrun(PICFILE *f, void *vbuf){
	unsigned char *buf=vbuf, *bufp, *runp, *erunp;
	int i, j, n, slack;
	if(f->line==0){
		f->buf=malloc(NBUF);
		if(f->buf==0){
			_PICerror="Can't allocate buffer";
			return 0;
		}
		f->ebuf=f->bufp=f->buf;
	}
	if(f->line==f->height){
		_PICerror="Read past end of picture";
		return 0;
	}
	bufp=buf;
	for(i=0;i!=f->width;){
		while(f->ebuf-f->bufp<f->nchan+1){	/* read until buffer has complete runs */
			slack=f->ebuf-f->bufp;
			runp=f->buf;
			while(f->bufp!=f->ebuf)
				*runp++ = *f->bufp++;
			n=read(f->fd, runp, NBUF-slack);
			if(n<=0){
				if(n==0) _PICerror="EOF reading picture";
				return 0;
			}
			f->ebuf=runp+n;
			f->bufp=f->buf;
		}
		n=*f->bufp++;
		i+=n+1;
		if(i>f->width){
			_PICerror="run spans scan lines";
			return 0;
		}
		runp=bufp;
		for(j=0;j!=f->nchan;j++)
			*bufp++=*f->bufp++;
		erunp=bufp+n*f->nchan;
		while(bufp!=erunp)
			*bufp++ = *runp++;
	}
	f->line++;
	return 1;
}
int _PWRrun(PICFILE *f, void *vbuf){
	unsigned char *buf=vbuf, *last, *runp, *pix, *next;
	int nrun, nchan, i;
	if(f->line==0){
		f->buf=malloc(f->width*(f->nchan+1));	/* big enough for any scan line */
		if(f->buf==0){
			_PICerror="Can't allocate buffer";
			return 0;
		}
		_PWRheader(f);
	}
	if(f->line>=f->height){
		_PICerror="Write past end of picture";
		return 0;
	}
	f->line++;
	nchan=f->nchan;
	last=buf+nchan*(f->width-1);
	runp=f->buf;
	while(buf<=last){
		pix=buf;
		next=buf+nchan;
		while(next<=last && *pix==*next++)
			pix++;
		nrun=(pix-buf)/nchan;
		pix=buf;
		buf+=nrun*nchan+nchan;
		while(nrun>=255){
			*runp++=255;
			i=nchan;
			do
				*runp++=*pix++;
			while(--i>0);
			pix-=nchan;
			nrun-=256;
		}
		if(nrun>=0){
			*runp++=nrun;
			i=nchan;
			do
				*runp++=*pix++;
			while(--i>0);
		}
	}
	return write(f->fd, f->buf, runp-f->buf)==runp-f->buf;
}
void _PCLrun(PICFILE *f){
	if(f->buf) free(f->buf);
}
