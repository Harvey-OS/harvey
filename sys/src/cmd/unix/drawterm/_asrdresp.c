#include "lib9.h"
#include "auth.h"
#include "authlocal.h"

static char *pbmsg = "AS protocol botch";

int
_asrdresp(int fd, char *buf, int len)
{
	char error[ERRLEN];

	if(read(fd, buf, 1) != 1){
		werrstr(pbmsg);
		return -1;
	}

	switch(buf[0]){
	case AuthOK:
		if(_asreadn(fd, buf, len) < 0){
			werrstr(pbmsg);
			return -1;
		}
		break;
	case AuthErr:
		if(_asreadn(fd, error, ERRLEN) < 0){
			werrstr(pbmsg);
			return -1;
		}
		error[ERRLEN-1] = 0;
		werrstr(error);
		return -1;
	default:
		werrstr(pbmsg);
		return -1;
	}
	return 0;
}
