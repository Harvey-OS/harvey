#include <stddef.h>
#include <grp.h>

extern int _getpw(int *, char **, char **);
extern char **_grpmems(char *);

static struct group holdgroup;

struct group *
getgrgid(gid_t gid)
{
	int num;
	char *nam, *mem;

	num = gid;
	nam = 0;
	mem = 0;
	if(_getpw(&num, &nam, &mem)){
		holdgroup.gr_name = nam;
		holdgroup.gr_gid = num;
		holdgroup.gr_mem = _grpmems(mem);
		return &holdgroup;
	}
	return NULL;
}
