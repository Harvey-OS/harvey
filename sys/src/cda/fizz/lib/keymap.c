#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

int
f_keymap(Keymap *t, char **s)
{
	register char *a, *b;

	while(t->id){
		for(a = *s, b = t->id; *a == *b; a++, b++)
			if(!*a || !*b) break;
		if((*b == 0) && ((*a == 0) || (!ISNM(*a))))
			break;
		t++;
	}
	if(t->id == 0)
		return(-1);
	if(*a){
		while(ISNM(*a)) a++;
		while(ISBL(*a)) a++;
	}
	*s = a;
	/*print("keymap(%s) -> %s\n", *s, t->id? t->id:"(null)");/**/
	return(t->index);
}
