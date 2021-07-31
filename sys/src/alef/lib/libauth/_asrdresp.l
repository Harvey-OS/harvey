#include <alef.h>
#include <auth.h>
#include "authlocal.h"

intern byte pbmsg[] = "AS protocol botch";

int
_asrdresp(int fd, byte *buf, int len)
{
	byte error[ERRLEN];

	rescue {
		werrstr(pbmsg);
		return -1;
	}
		
	if(read(fd, buf, 1) != 1)
		raise;

	switch(buf[0]){
	case AuthOK:
		if(_asreadn(fd, buf, len) < 0)
			raise;
		break;
	case AuthErr:
		if(_asreadn(fd, error, ERRLEN) < 0)
			raise;
		error[ERRLEN-1] = 0;
		werrstr(error);
		return -1;
	default:
		raise;
	}
	return 0;
}
