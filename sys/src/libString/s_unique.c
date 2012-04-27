#include <u.h>
#include <libc.h>
#include "String.h"

String*
s_unique(String *s)
{
	String *p;

	if(s->ref > 1){
		p = s;
		s = s_clone(p);
		s_free(p);
	}
	return s;
}
