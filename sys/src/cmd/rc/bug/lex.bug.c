#include <u.h>
#include <libc.h>

#define EOF (-1)

// typedef Rune Runeoreof;		/* Rune is uint */
typedef int Runeoreof;

Runeoreof getnext(void);

Runeoreof future = EOF;

Runeoreof
getnext(void)
{
	return 'A';
}

Runeoreof
nextc(void)
{
	if(future==EOF)
		future = getnext();
	return future;
}

void
main(void)
{
	print("%x\n", nextc());
	exits(0);
}
