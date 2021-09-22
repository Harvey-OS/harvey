#include	<ctype.h>

int
toupper(int c)
{
	return (c < 'a' || c > 'z')? c: _toupper(c);
}

int
tolower(int c)
{
	return (c < 'A' || c > 'Z')? c: _tolower(c);
}
