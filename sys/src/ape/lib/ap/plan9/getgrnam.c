#include <stddef.h>
#include <grp.h>

extern int _getpw(int *, char **, char **);
extern char **_grpmems(char *);

static struct group holdgroup;

struct group *
getgrnam(const char *name)
{
	int num;
	char *nam, *mem;

	num = 0;
	nam = name;
	mem = 0;
	if(_getpw(&num, &nam, &mem)){
		holdgroup.gr_name = nam;
		holdgroup.gr_gid = num;
		holdgroup.gr_mem = _grpmems(mem);
		return &holdgroup;
	}
	return NULL;
}
