#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authlocal.h"

static char *abmsg = "/dev/authenticate: %r";
static char *cgmsg = "client gave up";

int
srvauth(int fd, char *user)
{
	int n, afd;
	char trbuf[TICKREQLEN];
	char tbuf[2*TICKETLEN];

	/* get ticket request from kernel and pass to client */
	afd = open("/dev/authenticate", ORDWR);
	if(afd < 0){
		werrstr(abmsg);
		return -1;
	}
	n = read(afd, trbuf, TICKREQLEN);
	if(n != TICKREQLEN){
		close(afd);
		werrstr(abmsg);
		return -1;
	}
	if(write(fd, trbuf, TICKREQLEN) < 0){
		close(afd);
		werrstr(cgmsg);
		return -1;
	}

	/* get reply from client and pass to kernel */
	if(_asreadn(fd, tbuf, TICKETLEN+AUTHENTLEN) < 0){
		close(afd);
		werrstr(cgmsg);
		return -1;
	}
	if(write(afd, tbuf, TICKETLEN+AUTHENTLEN) < 0){
		close(afd);
		memset(tbuf, 0, AUTHENTLEN);
		write(fd, tbuf, AUTHENTLEN);
		werrstr("permission denied");
		return -1;
	}

	/* get authenticator from kernel and pass to client*/
	read(afd, tbuf, AUTHENTLEN);
	close(afd);
	if(write(fd, tbuf, AUTHENTLEN) < 0){
		werrstr("permission denied");
		return -1;
	}

	strcpy(user, getuser());
	return 0;
}
