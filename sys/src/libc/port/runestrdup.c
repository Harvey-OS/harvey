#include <u.h>
#include <libc.h>

Rune*
runestrdup(Rune *s) 
{  
	Rune *ns;

	ns = malloc(sizeof(Rune)*(runestrlen(s) + 1));
	if(ns == 0)
		return 0;

	return runestrcpy(ns, s);
}
