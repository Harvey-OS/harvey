#include "lib.h"
#include <stddef.h>
#include <pwd.h>
#include <string.h>

static struct passwd holdpw;
static char dirbuf[40] = "/usr/";
static char *rc = "/bin/rc";

struct passwd *
getpwnam(const char *name)
{
	int num;
	char *nam, *mem;

	num = 0;
	nam = name;
	mem = 0;
	if(_getpw(&num, &nam, &mem)){
		holdpw.pw_name = nam;
		holdpw.pw_uid = num;
		holdpw.pw_gid = num;
		strncpy(dirbuf+5, nam, sizeof(dirbuf)-6);
		holdpw.pw_dir = dirbuf;
		holdpw.pw_shell = rc;
		return &holdpw;
	}
	return NULL;
}
