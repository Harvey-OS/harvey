#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authlocal.h"

static char *pbmsg = "AS protocol botch";

int
_asrdresp(int fd, char *buf, int len)
{
	int n;
	char error[ERRLEN];

	if(read(fd, buf, 1) != 1){
		werrstr(pbmsg);
		return -1;
	}

	n = len;
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
		werrstr("remote: %s", error);
		return -1;
	case AuthOKvar:
		if(_asreadn(fd, error, 5) < 0){
			werrstr(pbmsg);
			return -1;
		}
		error[5] = 0;
		n = atoi(error);
		if(n <= 0 || n > len){
			werrstr(pbmsg);
			return -1;
		}
		memset(buf, 0, len);
		if(_asreadn(fd, buf, n) < 0){
			werrstr(pbmsg);
			return -1;
		}
		break;
	default:
		werrstr(pbmsg);
		return -1;
	}
	return n;
}
