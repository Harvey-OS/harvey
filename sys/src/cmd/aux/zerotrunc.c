/*
 * cat standard input until you get a zero byte
 */

#include <u.h>
#include <libc.h>
#include <bio.h>

void
main(void)
{
	int c;
	Biobuf bin, bout;

	Binit(&bin, 0, OREAD);
	Binit(&bout, 1, OWRITE);
	while((c = Bgetc(&bin)) != Beof && c != 0){
		Bputc(&bout, c);
	}
	Bflush(&bout);
	exits(0);
}

