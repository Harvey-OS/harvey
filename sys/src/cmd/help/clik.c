#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include "dat.h"
#include "fns.h"

Rune *left[] = {
	L"{[(<",
	L"\n",
	L"'\"`",
	0
};
Rune *right[] = {
	L"}])>",
	L"\n",
	L"'\"`",
	0
};

ulong	cp;

int
tgetc(Text *t)
{
	if(cp < t->n)
		return t->s[cp++];
	return 0;
}

int
tbgetc(Text *t)
{
	if(cp > 0)
		return t->s[--cp];
	return 0;
}


int
clickmatch(Text *t, int cl, int cr, int dir)
{
	int c, nest;

	nest = 1;
	while((c=(dir>0? tgetc(t) : tbgetc(t)))>0)
		if(c == cr){
			if(--nest == 0)
				return 1;
		}else if(c == cl)
			nest++;
	return cl=='\n' && nest==1;
}

Rune*
strrune(Rune *s, Rune c)
{
	Rune c1;

	if(c == 0) {
		while(*s++)
			;
		return s-1;
	}

	while(c1 = *s++)
		if(c1 == c)
			return s-1;
	return 0;
}

void
doubleclick(Text *t, ulong p0)
{
	int c, i;
	Rune *r, *l;

	t->q0 = t->q1 = p0;
	for(i=0; left[i]; i++){
		l = left[i];
		r = right[i];
		/* try left match */
		if(p0 == 0){
			cp = p0;
			c = '\n';
		}else{
			cp = p0-1;
			c = tgetc(t);
		}
		if(strrune(l, c)){
			if(clickmatch(t, c, r[strrune(l, c)-l], 1)){
				t->q0 = p0;
				t->q1 = cp-(c!='\n');
			}
			return;
		}
		/* try right match */
		if(p0 == t->n){
			cp = p0;
			c='\n';
		}else{
			cp =  p0+1;
			c = tbgetc(t);
		}
		if(strrune(r, c)){
			if(clickmatch(t, c, l[strrune(r, c)-r], -1)){
				t->q0 = cp;
				if(c!='\n' || cp!=0 || (cp=0,tgetc(t))=='\n')
					t->q0++;
				t->q1 = p0+(p0<t->n && c=='\n');
			}
			return;
		}
	}
	/* try filling out word to right */
	cp = p0;
	while(alnum(tgetc(t)))
		t->q1++;
	/* try filling out word to left */
	cp = p0;
	while(alnum(tbgetc(t)))
		t->q0--;
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
