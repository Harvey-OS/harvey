#include <u.h>
#include <libc.h>
#include <bio.h>

void
main(int argc, char **argv)
{
	Biobuf b;
	char *s;
	int n;


	n = atoi(argv[1]);
	s = malloc(n+1);
	memset(s, 'a', n);
	s[n] = '\0';
	Binit(&b, 1, OWRITE);
	Bprint(&b, "%s\n", s);
	Bflush(&b);
}
