#include <u.h>
#include <libc.h>

Rune*
runestrecpy(Rune *s1, Rune *es1, Rune *s2)
{
	if(s1 >= es1)
		return s1;

	while(*s1++ = *s2++){
		if(s1 == es1){
			*--s1 = '\0';
			break;
		}
	}
	return s1;
}
