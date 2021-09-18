#include "lib9.h"


long
runestrlen(Rune *s)
{
	int i;

	i = 0;
	while(*s++)
		i++;
	return i;
}
