#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

char *trivial[] = {
	"login",
	"guest",
	"change me",
	"passwd",
	"no passwd",
	"anonymous",
	0
};

static int
userexists(char *u)
{
	char buf[sizeof KEYDB + NAMELEN + 6], dir[DIRLEN];

	sprint(buf, "%s/%s", KEYDB, u);
	return stat(buf, dir) >= 0;
}

int
okpasswd(char *p)
{
	char passwd[11];
	char back[11];
	int i, n;

	strncpy(passwd, p, sizeof p - 1);
	passwd[sizeof passwd - 1] = '\0';
	n = strlen(passwd);
	while(passwd[n - 1] == ' ')
		n--;
	passwd[n] = '\0';
	for(i = 0; i < n; i++)
		back[i] = passwd[n - 1 - i];
	back[n] = '\0';

	for(i = 0; trivial[i]; i++)
		if(strcmp(passwd, trivial[i]) == 0
		|| strcmp(back, trivial[i]) == 0)
			return 0;

	if(userexists(passwd))
		return 0;
	if(userexists(back))
		return 0;
	return 1;
}
