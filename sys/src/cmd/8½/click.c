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
doubleclick(Window *w, ulong p0)
{
	int c, i;
	Rune *r, *l;

	w->q0 = w->q1 = p0;
	for(i=0; left[i]; i++){
		l = left[i];
		r = right[i];
		/* try left match */
		if(p0 == 0){
			cp = p0;
			c = '\n';
		}else{
			cp = p0-1;
			c = wgetc(w);
		}
		if(strrune(l, c)){
			if(clickmatch(w, c, r[strrune(l, c)-l], 1)){
				w->q0 = p0;
				w->q1 = cp-1;
				if(c=='\n' && (cp--,wgetc(w))!=0x04)	/* EOT is trouble */
					w->q1++;
			}
			return;
		}
		/* try right match */
		if(p0 == w->text.n){
			cp = p0;
			c='\n';
		}else{
			cp =  p0+1;
			c = wbgetc(w);
		}
		if(strrune(r, c)){
			if(clickmatch(w, c, l[strrune(r, c)-r], -1)){
				w->q0 = cp;
				if(c!='\n' || cp!=0 || (cp=0,wgetc(w))=='\n')
					w->q0++;
				w->q1 = p0+(p0<w->text.n && c=='\n');
			}
			return;
		}
	}
	/* try filling out word to right */
	cp = p0;
	while(alnum(wgetc(w)))
		w->q1++;
	/* try filling out word to left */
	cp = p0;
	while(alnum(wbgetc(w)))
		w->q0--;
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
