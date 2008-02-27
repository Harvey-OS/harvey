#include <u.h>
#include <libc.h>
#include <authsrv.h>
#include <bio.h>
#include "authcmdlib.h"

static int
getkey(char *authkey)
{
	Nvrsafe safe;

	if(readnvram(&safe, 0) < 0)
		return -1;
	memmove(authkey, safe.machkey, DESKEYLEN);
	memset(&safe, 0, sizeof safe);
	return 0;
}

int
getauthkey(char *authkey)
{
	if(getkey(authkey) == 0)
		return 1;
	print("can't read NVRAM, please enter machine key\n");
	getpass(authkey, nil, 0, 1);
	return 1;
}
