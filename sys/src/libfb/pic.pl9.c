/*
 * Type-specific code for TYPE=pl9
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#define	NBB	8		/* number of bits per byte */
#define	MAXB	((1<<NBB)-1)	/* maximum byte */
int _PRDpl9(PICFILE *f, void *vbuf){
	unsigned char *buf=vbuf, *ebuf, *bp;
	char *ldepth;
	int npb, nbl, mask, mul, sh;
	if(f->line==0){
		ldepth=picgetprop(f, "LDEPTH");
		if(ldepth==0){
			_PICerror="no LDEPTH";
			return 0;
		}
		f->depth=1<<atoi(ldepth);
		if(f->depth<=0 || NBB<f->depth){
			_PICerror="bad LDEPTH";
			return 0;
		}
	}
	npb=NBB/f->depth;
	nbl=(f->x+f->width+npb-1)/npb-f->x/npb;
	mask=(1<<f->depth)-1;
	mul=MAXB/mask;
	if(f->buf==0){
		f->buf=malloc(nbl);
		if(f->buf==0){
			_PICerror="Can't allocate read buffer";
			return 0;
		}
	}
	f->line++;
	if(_PICread(f->fd, f->buf, nbl)<=0) return 0;
	bp=f->buf;
	sh=NBB-f->depth*(f->x%npb+1);
	for(ebuf=buf+f->width;buf!=ebuf;buf++){
		*buf=MAXB-((*bp>>sh)&mask)*mul;
		sh-=f->depth;
		if(sh<0){
			bp++;
			sh=NBB-f->depth;
		}
	}
	return 1;
}
int _PWRpl9(PICFILE *f, void *vbuf){
	unsigned char *buf=vbuf, *ebuf, *bp;
	char hdr[12*5+1], *ldepth;
	int npb, nbl, mask, sh;
	if(f->line==0){
		ldepth=picgetprop(f, "LDEPTH");
		if(ldepth==0){
			_PICerror="no LDEPTH";
			return 0;
		}
		f->depth=1<<atoi(ldepth);
		if(f->depth<=0 || NBB<f->depth){
			_PICerror="bad LDEPTH";
			return 0;
		}
		sprint(hdr, "%11d %11d %11d %11d %11d ",
			atoi(ldepth), f->x, f->y, f->x+f->width, f->y+f->height);
		write(f->fd, hdr, 5*12);
	}
	npb=NBB/f->depth;
	nbl=(f->x+f->width+npb-1)/npb-f->x/npb;
	mask=(~0<<(NBB-f->depth))&MAXB;
	if(f->buf==0){
		f->buf=malloc(nbl+1);	/* +1 because loop below is sloppy */
		if(f->buf==0){
			_PICerror="can't allocate write buffer";
			return 0;
		}
	}
	f->line++;
	bp=f->buf;
	*bp=0;
	sh=f->depth*(f->x%npb);
	for(ebuf=buf+f->width;buf!=ebuf;buf++){
		*bp|=(~*buf&mask)>>sh;
		sh+=f->depth;
		if(sh==NBB){
			sh=0;
			*++bp=0;
		}
	}
	return write(f->fd, f->buf, nbl)==nbl;
}
void _PCLpl9(PICFILE *f){
	if(f->buf) free(f->buf);
}
int _PICplan9header(PICFILE *p, char *buf){
	char *bp, *ebp;
	ebp=&buf[5*12];
	for(bp=buf;bp!=ebp;bp++) if((*bp<'0' || '9'<*bp) && *bp!=' ') return 0;
	*ebp='\0';
	picputprop(p, "TYPE", "plan9");
	picputprop(p, "WINDOW", buf+12);
	picputprop(p, "NCHAN", "1");
	picputprop(p, "CHAN", "m");
	buf[12]='\0';
	picputprop(p, "LDEPTH", buf);
	return 1;
}
