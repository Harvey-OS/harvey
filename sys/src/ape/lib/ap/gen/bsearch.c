#include <stdlib.h>
#include <stdio.h>
void*
bsearch(const void* key, const void* base, size_t nmemb, size_t size,
		int (*compar)(const void*, const void*))
{
	long i, low, high;
	int r;

	for(low=-1, high=nmemb; high>low+1; ){
		i=(high+low)/2;
		if((r=(*compar)(key, ((char *)base)+i*size))<0)
			high=i;
		else if(r==0)
			return ((char *)base)+i*size;
		else
			low=i;
	}
	if((*compar)(key, ((char *)base)+low*size)==0)
		return ((char *)base)+low*size;
	return 0;
}
