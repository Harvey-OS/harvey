#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

void
configrc(Method *)
{
	execl("/rc", "/rc", "-m", "/rcmain", "-i", 0);
	fatal("rc");
}

int
connectrc(void)
{
	// does not get here
	return -1;
}
