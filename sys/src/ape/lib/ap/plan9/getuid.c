#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

uid_t
getuid(void)
{
	struct passwd *p;
	p = getpwnam(getlogin());
	return p? p->pw_uid : 1;
}

uid_t
geteuid(void)
{
	return getuid();
}
