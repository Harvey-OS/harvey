#include <alef.h>

intern byte user[NAMELEN+1];

byte *
getuser(void)
{
	int fd, n;

	fd = open("/dev/user", OREAD);
	if(fd < 0)
		return "none";
	n = read(fd, user, (sizeof user)-1);
	close(fd);
	if(n <= 0)
		strcpy(user, "none");
	else
		user[n] = 0;
	return user;
}
