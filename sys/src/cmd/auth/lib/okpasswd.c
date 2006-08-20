#include <u.h>
#include <libc.h>
#include <authsrv.h>
#include <bio.h>
#include "authcmdlib.h"

char *trivial[] = {
	"login",
	"guest",
	"change me",
	"passwd",
	"no passwd",
	"anonymous",
	0
};

char*
okpasswd(char *p)
{
	char passwd[ANAMELEN];
	char back[ANAMELEN];
	int i, n;

	strncpy(passwd, p, sizeof passwd - 1);
	passwd[sizeof passwd - 1] = '\0';
	n = strlen(passwd);
	while(passwd[n - 1] == ' ')
		n--;
	passwd[n] = '\0';
	for(i = 0; i < n; i++)
		back[i] = passwd[n - 1 - i];
	back[n] = '\0';
	if(n < 8)
		return "password must be at least 8 chars";

	for(i = 0; trivial[i]; i++)
		if(strcmp(passwd, trivial[i]) == 0
		|| strcmp(back, trivial[i]) == 0)
			return "trivial password";

	return 0;
}
