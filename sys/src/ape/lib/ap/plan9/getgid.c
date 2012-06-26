#include <sys/types.h>
#include <grp.h>
#include <unistd.h>

/*
 * BUG: assumes group that is same as user name
 * is the one wanted (plan 9 has no "current group")
 */
gid_t
getgid(void)
{
	struct group *g;
	g = getgrnam(getlogin());
	return g? g->gr_gid : 1;
}

gid_t
getegid(void)
{
	return getgid();
}
