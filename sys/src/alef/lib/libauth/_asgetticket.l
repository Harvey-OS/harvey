#include <alef.h>
#include <auth.h>
#include "authlocal.h"

intern byte pbmsg[] = "AS protocol botch";

int
_asgetticket(int fd, byte *trbuf, byte *tbuf)
{
	if(write(fd, trbuf, TICKREQLEN) < 0){
		close(fd);
		werrstr(pbmsg);
		return -1;
	}
	return _asrdresp(fd, tbuf, 2*TICKETLEN);
}
