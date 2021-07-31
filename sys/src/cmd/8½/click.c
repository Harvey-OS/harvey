#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include <layer.h>
#include "dat.h"
#include "fns.h"

Rune *left[]= {
	L"{[(<«",
	L"\n",
	L"'\"`",
	0
};
Rune *right[]= {
	L"}])>»",
	L"\n",
	L"'\"`",
	0
};

ulong	cp;

int
wgetc(Window *w)
{
	if(cp < w->text.n)
		return w->text.s[cp++];
	return 0;
}

int
wbgetc(Window *w)
{
	if(cp > 0)
		return w->text.s[--cp];
	return 0;
}


int
clickmatch(Window *w, int cl, int cr, int dir)
{
	int c, nest;

	nest=1;
	while((c=(dir>0? wgetc(w) : wbgetc(w)))>0){
		if(cl=='\n' && c==0x04)	/* EOT is trouble */
			return 1;
		if(c == cr){
			if(--nest == 0)
				return 1;
		}else if(c == cl)
			nest++;
	}
	return cl=='\n' && nest==1;
}

void
doubleclick(Window *w, long *q0, long *q1)
{
	int c, i;
	Rune *r, *l;
	long q;

	q = 0;
	for(i=0; left[i]; i++){
		q = *q0;
		l = left[i];
		r = right[i];
		/* try left match */
		if(q == 0){
			cp = q;
			c = '\n';
		}else{
			cp = q-1;
			c = wgetc(w);
		}
		if(strrune(l, c)){
			if(clickmatch(w, c, r[strrune(l, c)-l], 1)){
				*q1 = cp-1;
				if(c=='\n' && (cp--,wgetc(w))!=0x04)	/* EOT is trouble */
					(*q1)++;
			}
			return;
		}
		/* try right match */
		if(q == w->text.n){
			cp = q;
			c='\n';
		}else{
			cp =  q+1;
			c = wbgetc(w);
		}
		if(strrune(r, c)){
			if(clickmatch(w, c, l[strrune(r, c)-r], -1)){
				*q0 = cp;
				if(c!='\n' || cp!=0 || (cp=0,wgetc(w))=='\n')
					(*q0)++;
				*q1 = q+(q<w->text.n && c=='\n');
			}
			return;
		}
	}
	/* try filling out word to right */
	cp = q;
	while(alnum(wgetc(w)))
		(*q1)++;
	/* try filling out word to left */
	cp = q;
	while(alnum(wbgetc(w)))
		(*q0)--;
}

int
alnum(int c)
{
	/*
	 * Hard to get absolutely right.  Use what we know about ASCII
	 * and assume anything above the Latin control characters is
	 * potentially an alphanumeric.
	 */
	if(c <= ' ')
		return 0;
	if(0x7F<=c && c<=0xA0)
		return 0;
	if(utfrune("!\"#$%&'()*+,-./:;<=>?@`[\\]^{|}~", c))
		return 0;
	return 1;
}
