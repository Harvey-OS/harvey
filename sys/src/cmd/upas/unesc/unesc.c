/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
