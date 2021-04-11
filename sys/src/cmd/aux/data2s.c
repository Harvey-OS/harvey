/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>

void
main(int argc, char *argv[])
{
	Biobuf bin, bout;
	i32 len, slen;
	int c;

	if(argc != 2){
		fprint(2, "usage: data2s name\n");
		exits("usage");
	}
	Binit(&bin, 0, OREAD);
	Binit(&bout, 1, OWRITE);
	for(len=0; (c=Bgetc(&bin))!=Beof; len++){
		if((len&7) == 0)
			Bprint(&bout, "DATA %scode+%ld(SB)/8, $\"", argv[1], len);
		if(c)
			Bprint(&bout, "\\%uo", c);
		else
			Bprint(&bout, "\\z");
		if((len&7) == 7)
			Bprint(&bout, "\"\n");
	}
	slen = len;
	if(len & 7){
		while(len & 7){
			Bprint(&bout, "\\z");
			len++;
		}
		Bprint(&bout, "\"\n");
	}
	Bprint(&bout, "GLOBL %scode+0(SB), $%ld\n", argv[1], len);
	Bprint(&bout, "GLOBL %slen+0(SB), $4\n", argv[1]);
	Bprint(&bout, "DATA %slen+0(SB)/4, $%ld\n", argv[1], slen);
	exits(0);
}
