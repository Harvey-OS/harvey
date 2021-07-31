#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

int
ishost(char *user)
{
	char buf[DIRLEN];
	char name[sizeof KEYDB+2*NAMELEN+6];

	sprint(name, "%s/%s/ishost", KEYDB, user);
	return stat(name, buf) >= 0;
}
