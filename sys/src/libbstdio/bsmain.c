/*
 * stdio emulation via bio.
 */
#include <u.h>
#include <libc.h>
#include <bstdio.h>

#undef main

void
main(int argc, char **argv)
{
	Binit(stdin,  0, OREAD);
	Binit(stdout, 1, OWRITE);
	Binit(stderr, 2, OWRITE);
	_bs_main(argc, argv);
	exits(0);
}
