#include <alef.h>

intern char user[NAMELEN+1];

char *
getuser(void)
{
	int fd;
	int n;

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
