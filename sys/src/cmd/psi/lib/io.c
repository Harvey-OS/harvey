#include <u.h>
#include <libc.h>
#include "system.h"
# include "stdio.h"

# include "object.h"


FILE *
sopen(struct pstring pstring)
{
	FILE	*fp ;
	fp = (FILE *)vm_alloc(sizeof(*fp)) ;
	if ( fp == NULL )
		return(NULL) ;
	fp->fd = getdtablesize()+1 ;	/*plan9*/
	fp->buf = (char *)vm_alloc(pstring.length*sizeof(char)) ;
	fp->wp = fp->buf + pstring.length ;
	memmove(fp->buf,pstring.chars,pstring.length) ;
	fp->rp = fp->buf ;
	instring=1;
	return(fp) ;
}

int
stell(FILE *sp)
{
	return(sp->rp-sp->buf) ;
}

int
un_getc(int c,FILE *fp)
{
	if ( c == EOF )
		return(EOF) ;

	if(decryptf){
		if(d_unget != EOF)
			pserror("decrypt pushed back 2 chars", "un_getc internal");
		d_unget=c;
	}
	else if (instring){
		if ( fp->rp > fp->buf  &&  c == fp->rp[-1] )
			fp->rp-- ;
		else
			c = EOF ;
	}
	else{
		if ( c == '\n' )
			Line_no-- ;
		Char_ct--;
		c = ungetc(c,fp) ;
	}
	return(c) ;
}
int
f_getc(FILE *fp)
{
	register int	c ;
	if ( instring ){
		if ( fp->rp >= fp->wp ){
			c  = EOF ;
			/*fp->_flag |= _IOEOF ;*/
		}
		else
			c = (int)*fp->rp++ ;
	}
	else{
		c = fgetc(fp) ;
		if ( c == '\n' )
			Line_no++ ;
		Char_ct++;
	}
	return(c) ;
}

f_close(FILE *fp)
{
	if (instring){
		/*free(fp->buf) ;*/
		fp->buf = fp->rp = fp->wp = NULL ;
		instring=0;
		return(0) ;
	}
	else
		return(fclose(fp)) ;
}
