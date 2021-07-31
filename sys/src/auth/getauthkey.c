#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

#define	NVPASSWD	(1024+900)

int
getauthkey(char *authkey)
{
	Nvrsafe safe;
	int fd;

	fd = open("#r/nvram", OREAD);
	if(fd < 0)
		error("can't open nvram");
	if(seek(fd, NVPASSWD, 0) < 0 || read(fd, &safe, sizeof safe) != sizeof safe)
		error("can't read nvram");
	close(fd);
	if(nvcsum(safe.machkey, DESKEYLEN) != safe.machsum)
		return 0;
	memmove(authkey, safe.machkey, DESKEYLEN);
	return 1;
}
