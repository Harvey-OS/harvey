/*
 * picpack -- ansi version
 * Bugs:
 *	f and d are non-portable
 *	could be faster
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <ape/stdarg.h>
#include <fb.h>
void picpack(PICFILE *f, char *buf, char *fmt, ...){
	register char *bp, *cp;
	register short *sp;
	register long *lp;
	register float *fp;
	register double *dp;
	register char *ebuf;
	int nchan;
	va_list pvar;
	va_start(pvar, fmt);
	nchan=f->nchan;
	ebuf=buf+f->width*nchan;
	for(;*fmt;fmt++) switch(*fmt){
	case 'c':
		cp=va_arg(pvar, char *);
		for(bp=buf;bp!=ebuf;bp+=nchan)
			*bp=*cp++;
		buf++;
		ebuf++;
		break;
	case 's':
		sp=va_arg(pvar, short *);
		for(bp=buf;bp!=ebuf;bp+=nchan){
			*bp=*sp;
			bp[1]=*sp++>>8;
		}
		buf+=2;
		ebuf+=2;
		break;
	case 'l':
		lp=va_arg(pvar, long *);
		for(bp=buf;bp!=ebuf;bp+=nchan){
			*bp=*lp;
			bp[1]=*lp>>8;
			bp[2]=*lp>>16;
			bp[3]=*lp++>>24;
		}
		buf+=4;
		ebuf+=4;
		break;
	case 'f':
		fp=va_arg(pvar, float *);
		for(bp=buf;bp!=ebuf;bp+=nchan){
			cp=(char *)fp++;
			bp[0]=cp[0];
			bp[1]=cp[1];
			bp[2]=cp[2];
			bp[3]=cp[3];
		}
		buf+=4;
		ebuf+=4;
		break;
	case 'd':
		dp=va_arg(pvar, double *);
		for(bp=buf;bp!=ebuf;bp+=nchan){
			cp=(char *)dp++;
			bp[0]=cp[0];
			bp[1]=cp[1];
			bp[2]=cp[2];
			bp[3]=cp[3];
			bp[4]=cp[4];
			bp[5]=cp[5];
			bp[6]=cp[6];
			bp[7]=cp[7];
		}
		buf+=8;
		ebuf+=8;
		break;
	case '_':
		buf++;
		ebuf++;
		break;
	}
	va_end(pvar);
}
