/*
 * pANS stdio -- vfscanf
 */
#include "iolib.h"
#include <ctype.h>
static int icvt_f(FILE *f, va_list *args, int store, int width, int type);
static int icvt_x(FILE *f, va_list *args, int store, int width, int type);
static int icvt_sq(FILE *f, va_list *args, int store, int width, int type);
static int icvt_c(FILE *f, va_list *args, int store, int width, int type);
static int icvt_d(FILE *f, va_list *args, int store, int width, int type);
static int icvt_i(FILE *f, va_list *args, int store, int width, int type);
static int icvt_n(FILE *f, va_list *args, int store, int width, int type);
static int icvt_o(FILE *f, va_list *args, int store, int width, int type);
static int icvt_p(FILE *f, va_list *args, int store, int width, int type);
static int icvt_s(FILE *f, va_list *args, int store, int width, int type);
static int icvt_u(FILE *f, va_list *args, int store, int width, int type);
static int (*icvt[])(FILE *, va_list *, int, int, int)={
0,	0,	0,	0,	0,	0,	0,	0,	/* ^@ ^A ^B ^C ^D ^E ^F ^G */
0,	0,	0,	0,	0,	0,	0,	0,	/* ^H ^I ^J ^K ^L ^M ^N ^O */
0,	0,	0,	0,	0,	0,	0,	0,	/* ^P ^Q ^R ^S ^T ^U ^V ^W */
0,	0,	0,	0,	0,	0,	0,	0,	/* ^X ^Y ^Z ^[ ^\ ^] ^^ ^_ */
0,	0,	0,	0,	0,	0,	0,	0,	/* sp  !  "  #  $  %  &  ' */
0,	0,	0,	0,	0,	0,	0,	0,	/*  (  )  *  +  ,  -  .  / */
0,	0,	0,	0,	0,	0,	0,	0,	/*  0  1  2  3  4  5  6  7 */
0,	0,	0,	0,	0,	0,	0,	0,	/*  8  9  :  ;  <  =  >  ? */
0,	0,	0,	0,	0,	icvt_f,	0,	icvt_f,	/*  @  A  B  C  D  E  F  G */
0,	0,	0,	0,	0,	0,	0,	0,	/*  H  I  J  K  L  M  N  O */
0,	0,	0,	0,	0,	0,	0,	0,	/*  P  Q  R  S  T  U  V  W */
icvt_x,	0,	0,	icvt_sq,0,	0,	0,	0,	/*  X  Y  Z  [  \  ]  ^  _ */
0,	0,	0,	icvt_c,	icvt_d,	icvt_f,	icvt_f,	icvt_f,	/*  `  a  b  c  d  e  f  g */
0,	icvt_i,	0,	0,	0,	0,	icvt_n,	icvt_o,	/*  h  i  j  k  l  m  n  o */
icvt_p,	0,	0,	icvt_s,	0,	icvt_u,	0,	0,	/*  p  q  r  s  t  u  v  w */
icvt_x,	0,	0,	0,	0,	0,	0,	0,	/*  x  y  z  {  |  }  ~ ^? */

0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,

};
#define	ngetc(f)		(nread++, getc(f))
#define	nungetc(c, f)		(--nread, ungetc((c), f))
#define	wgetc(c, f, out)	if(width--==0) goto out; (c)=ngetc(f)
#define	wungetc(c, f)		(++width, nungetc(c, f))
static int nread, ncvt;
static const char *fmtp;

int vfscanf(FILE *f, const char *s, va_list args){
	int c, width, type, store;
	nread=0;
	ncvt=0;
	fmtp=s;
	for(;*fmtp;fmtp++) switch(*fmtp){
	default:
		if(isspace(*fmtp)){
			do
				c=ngetc(f);
			while(isspace(c));
			if(c==EOF) return ncvt?ncvt:EOF;
			nungetc(c, f);
			break;
		}
	NonSpecial:
		c=ngetc(f);
		if(c==EOF) return ncvt?ncvt:EOF;
		if(c!=*fmtp){
			nungetc(c, f);
			return ncvt;
		}
		break;
	case '%':
		fmtp++;
		if(*fmtp!='*') store=1;
		else{
			store=0;
			fmtp++;
		}
		if('0'<=*fmtp && *fmtp<='9'){
			width=0;
			while('0'<=*fmtp && *fmtp<='9') width=width*10 + *fmtp++ - '0';
		}
		else
			width=-1;
		type=*fmtp=='h' || *fmtp=='l' || *fmtp=='L'?*fmtp++:'n';
		if(!icvt[*fmtp]) goto NonSpecial;
		if(!(*icvt[*fmtp])(f, &args, store, width, type))
			return ncvt?ncvt:EOF;
		if(*fmtp=='\0') break;
		if(store) ncvt++;
	}
	return ncvt;	
}
static int icvt_n(FILE *f, va_list *args, int store, int width, int type){
#pragma ref f
#pragma ref width
	if(store){
		--ncvt;	/* this assignment doesn't count! */
		switch(type){
		case 'h': *va_arg(*args, short *)=nread; break;
		case 'n': *va_arg(*args, int *)=nread; break;
		case 'l':
		case 'L': *va_arg(*args, long *)=nread; break;
		}
	}
	return 1;
}
#define	SIGNED		1
#define	UNSIGNED	2
#define	POINTER		3
/*
 * Generic fixed-point conversion
 *	f is the input FILE *;
 *	args is the va_list * into which to store the number;
 *	store is a flag to enable storing;
 *	width is the maximum field width;
 *	type is 'h' 'l' or 'L', the scanf type modifier;
 *	unsgned is SIGNED, UNSIGNED or POINTER, giving part of the type to store in;
 *	base is the number base -- if 0, C number syntax is used.
 */
static int icvt_fixed(FILE *f, va_list *args,
				int store, int width, int type, int unsgned, int base){
	unsigned long int num=0;
	int sign=1, ndig=0, dig;
	int c;
	do
		c=ngetc(f);
	while(isspace(c));
	if(width--==0){
		nungetc(c, f);
		goto Done;
	}
	if(c=='+'){
		wgetc(c, f, Done);
	}
	else if(c=='-'){
		sign=-1;
		wgetc(c, f, Done);
	}
	switch(base){
	case 0:
		if(c=='0'){
			wgetc(c, f, Done);
			if(c=='x' || c=='X'){
				wgetc(c, f, Done);
				base=16;
			}
			else{
				ndig=1;
				base=8;
			}
		}
		else
			base=10;
		break;
	case 16:
		if(c=='0'){
			wgetc(c, f, Done);
			if(c=='x' || c=='X'){
				wgetc(c, f, Done);
			}
			else ndig=1;
		}
		break;
	}
	while('0'<=c && c<='9' || 'a'<=c && c<='f' || 'A'<=c && c<='F'){
		dig='0'<=c && c<='9'?c-'0':'a'<=c && c<='f'?c-'a'+10:c-'A'+10;
		if(dig>=base) break;
		ndig++;
		num=num*base+dig;
		wgetc(c, f, Done);
	}
	nungetc(c, f);
Done:
	if(ndig==0) return 0;
	if(store){
		switch(unsgned){
		case SIGNED:
			switch(type){
			case 'h': *va_arg(*args,  short *)=num*sign; break;
			case 'n': *va_arg(*args,  int *)=num*sign; break;
			case 'l':
			case 'L': *va_arg(*args,  long *)=num*sign; break;
			}
			break;
		case UNSIGNED:
			switch(type){
			case 'h': *va_arg(*args, unsigned short *)=num*sign; break;
			case 'n': *va_arg(*args, unsigned int *)=num*sign; break;
			case 'l':
			case 'L': *va_arg(*args, unsigned long *)=num*sign; break;
			}
			break;
		case POINTER:
			*va_arg(*args, void **)=(void *)(num*sign); break;
		}
	}
	return 1;
}
static int icvt_d(FILE *f, va_list *args, int store, int width, int type){
	return icvt_fixed(f, args, store, width, type, SIGNED, 10);
}
static int icvt_x(FILE *f, va_list *args, int store, int width, int type){
	return icvt_fixed(f, args, store, width, type, UNSIGNED, 16);
}
static int icvt_o(FILE *f, va_list *args, int store, int width, int type){
	return icvt_fixed(f, args, store, width, type, UNSIGNED, 8);
}
static int icvt_i(FILE *f, va_list *args, int store, int width, int type){
	return icvt_fixed(f, args, store, width, type, SIGNED, 0);
}
static int icvt_u(FILE *f, va_list *args, int store, int width, int type){
	return icvt_fixed(f, args, store, width, type, UNSIGNED, 10);
}
static int icvt_p(FILE *f, va_list *args, int store, int width, int type){
	return icvt_fixed(f, args, store, width, type, POINTER, 16);
}
#define	NBUF	509
static int icvt_f(FILE *f, va_list *args, int store, int width, int type){
	char buf[NBUF+1];
	char *s=buf;
	int c, ndig=0, ndpt=0, nexp=1;
	if(width<0 || NBUF<width) width=NBUF;	/* bug -- no limit specified in ansi */
	do
		c=ngetc(f);
	while(isspace(c));
	if(width--==0){
		nungetc(c, f);
		goto Done;
	}
	if(c=='+' || c=='-'){
		*s++=c;
		wgetc(c, f, Done);
	}
	while('0'<=c && c<='9' || ndpt==0 && c=='.'){
		if(c=='.') ndpt++;
		else ndig++;
		*s++=c;
		wgetc(c, f, Done);
	}
	if(c=='e' || c=='E'){
		*s++=c;
		nexp=0;
		wgetc(c, f, Done);
		if(c=='+' || c=='-'){
			*s++=c;
			wgetc(c, f, Done);
		}
		while('0'<=c && c<='9'){
			*s++=c;
			nexp++;
			wgetc(c, f, Done);
		}
	}
	nungetc(c, f);
Done:
	if(ndig==0 || nexp==0) return 0;
	*s='\0';
	if(store) switch(type){
	case 'h':
	case 'n': *va_arg(*args, float *)=atof(buf); break;
	case 'L': /* bug -- should store in a long double */
	case 'l': *va_arg(*args, double *)=atof(buf); break;
	}
	return 1;
}
static int icvt_s(FILE *f, va_list *args, int store, int width, int type){
#pragma ref type
	int c, nn;
	register char *s;
	if(store) s=va_arg(*args, char *);
	do
		c=ngetc(f);
	while(isspace(c));
	if(width--==0){
		nungetc(c, f);
		goto Done;
	}
	nn=0;
	while(!isspace(c)){
		if(c==EOF){
			nread--;
			if(nn==0) return 0;
			else goto Done;
		}
		nn++;
		if(store) *s++=c;
		wgetc(c, f, Done);
	}
	nungetc(c, f);
Done:
	if(store) *s='\0';
	return 1;
}
static int icvt_c(FILE *f, va_list *args, int store, int width, int type){
#pragma ref type
	int c;
	register char *s;
	if(store) s=va_arg(*args, char *);
	if(width<0) width=1;
	for(;;){
		wgetc(c, f, Done);
		if(c==EOF) return 0;
		if(store) *s++=c;
	}
Done:
	return 1;
}
static int match(int c, const char *pat){
	int ok=1;
	if(*pat=='^'){
		ok=!ok;
		pat++;
	}
	while(pat!=fmtp){
		if(pat+2<fmtp && pat[1]=='-'){
			if(pat[0]<=c && c<=pat[2]
			|| pat[2]<=c && c<=pat[0])
				return ok;
			pat+=2;
		}
		else if(c==*pat) return ok;
		pat++;
	}
	return !ok;
}
static int icvt_sq(FILE *f, va_list *args, int store, int width, int type){
#pragma ref type
	int c, nn;
	register char *s;
	register const char *pat;
	pat=++fmtp;
	if(*fmtp=='^') fmtp++;
	if(*fmtp!='\0') fmtp++;
	while(*fmtp!='\0' && *fmtp!=']') fmtp++;
	if(store) s=va_arg(*args, char *);
	nn=0;
	for(;;){
		wgetc(c, f, Done);
		if(c==EOF){
			nread--;
			if(nn==0) return 0;
			else goto Done;
		}
		if(!match(c, pat)) break;
		if(store) *s++=c;
		nn++;
	}
	nungetc(c, f);
Done:
	if(store) *s='\0';
	return 1;
}
