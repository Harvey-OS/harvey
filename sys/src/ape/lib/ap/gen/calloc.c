#include <stdlib.h>
#include <string.h>

void *
calloc(size_t nmemb, size_t size)
{
	void *mp;

	nmemb = nmemb*size;
	if(mp = malloc(nmemb))
		memset(mp, 0, nmemb);
	return(mp);
}
