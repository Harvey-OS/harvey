#include <u.h>
#include <libc.h>

void
main(int argc, char *argv[])
{
	if(argc>1)
		sleep(1000*atol(argv[1]));
	exits(0);
}
