#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"
/*
 * This is the same definition that 8½ uses
 */
int pl_idchar(int c){
	if(c<=' '
	|| 0x7F<=c && c<=0xA0
	|| utfrune("!\"#$%&'()*+,-./:;<=>?@`[\\]^{|}~", c))
		return 0;
	return 1;
}
int pl_rune1st(int c){
	return (c&0xc0)!=0x80;
}
char *pl_nextrune(char *s){
	do s++; while(!pl_rune1st(*s));
	return s;
}
int pl_runewidth(Font *f, char *s){
	char r[4], *t;
	t=r;
	do *t++=*s++; while(!pl_rune1st(*s));
	*t='\0';
	return strwidth(f, r);
}
