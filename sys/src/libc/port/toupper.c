#include	<ctype.h>

toupper(int c)
{

	if(c < 'a' || c > 'z')
		return c;
	return _toupper(c);
}

tolower(int c)
{

	if(c < 'A' || c > 'Z')
		return c;
	return _tolower(c);
}
