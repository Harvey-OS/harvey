#include <alef.h>
#include <auth.h>

int
amount(int fd, byte *old, int flag, byte *aname)
{
	if(authenticate(fd, -1) < 0)
		return -1;

	return mount(fd, old, flag, aname);
}
