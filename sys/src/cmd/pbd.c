#include <u.h>
#include <libc.h>

void
main(void)
{
	char buf[DIRLEN];

	if(stat(".", buf) < 0)
		write(1, "?", 1);
	write(1, buf, strlen(buf));
	exits(0);
}
