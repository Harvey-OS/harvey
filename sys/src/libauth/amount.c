#include <u.h>
#include <libc.h>
#include <auth.h>

int
amount(int fd, char *old, int flag, char *aname)
{
	if(authenticate(fd, -1) < 0)
		return -1;
	return mount(fd, old, flag, aname);
}
