#include <stdlib.h>
#include <stdio.h>
void*
bsearch(const void* key, const void* base, size_t nmemb, size_t size,
		int (*compar)(const void*, const void*))
{
	long i, bot, top, new;
	void *p;

	bot = 0;
	top = bot + nmemb - 1;
	while(bot <= top){
		new = (top + bot)/2;
		p = (char *)base+new*size;
		i = (*compar)(key, p);
		if(i == 0)
			return p;
		if(i > 0)
			bot = new + 1;
		else
			top = new - 1;
	}
	return 0;
}
