/*
 *	upas/unesc - interpret =?foo?bar?=char?= escapes
 */
#include <u.h>
#include <libc.h>
#include <bio.h>

int
hex(int c)
{
	if('0' <= c && c <= '9')
		return c - '0';
	if('a' <= c && c <= 'f')
		return c - 'a' + 10;
	if('A' <= c && c <= 'F')
		return c - 'A' + 10;
	return 0;
}

void
main(void)
{
	int c;
	Biobuf bin, bout;

	Binit(&bin,  0, OREAD);
	Binit(&bout, 1, OWRITE);
	while((c = Bgetc(&bin)) != Beof)
		if(c != '=')
			Bputc(&bout, c);
		else if((c = Bgetc(&bin)) != '?'){
			Bputc(&bout, '=');
			Bputc(&bout, c);
		} else {
			while((c = Bgetc(&bin)) != Beof && c != '?')
				continue;		/* consume foo */
			while((c = Bgetc(&bin)) != Beof && c != '?')
				continue;		/* consume bar */
			while((c = Bgetc(&bin)) != Beof && c != '?'){
				if(c == '='){
					c  = hex(Bgetc(&bin)) << 4;
					c |= hex(Bgetc(&bin));
				}
				Bputc(&bout, c);
			}
			c = Bgetc(&bin);		/* consume '=' */
			if (c != '=')	
				Bungetc(&bin);
		}
	Bterm(&bout);
	exits(0);
}
