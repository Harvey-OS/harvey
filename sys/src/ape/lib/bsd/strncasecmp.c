#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <bsd.h>

int
strncasecmp(char *s1, char *s2,int n)
{
	char *s1ptr, *s2ptr;
	int s1size, s2size, i;
	int ret_val;

	if (n<1)
		return 0;

	s1size = n<strlen(s1)?n+1:strlen(s1)+1;
	s2size = n<strlen(s2)?n+1:strlen(s2)+1;
	s1ptr = (char *)malloc(s1size); s2ptr = (char *)malloc(s2size);
	if ((s1ptr == NULL) || (s2ptr == NULL))
		return 0; /* any better ideas here? */

	for (i= 0; i < s1size ; i++ )
		s1ptr[i] = toupper(s1[i]);
	s1ptr[s1size] = '\0';

	for (i= 0; i < s2size ; i++ )
		s2ptr[i] = toupper(s2[i]);

	s2ptr[s2size] = '\0';
	ret_val = strncmp(s1ptr,s2ptr,n);
	free(s1ptr); free(s2ptr);
	return ret_val;
}
