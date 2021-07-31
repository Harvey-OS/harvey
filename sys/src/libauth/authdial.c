#include <u.h>
#include <libc.h>
#include <auth.h>


int
authdial(void)
{
	return dial("net!$auth!ticket", 0, 0, 0);
}
