#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authlocal.h"

static char *ccmsg = "can't connect to AS";

int
authenticate(int fd, int afd)
{
	int rv;
	char trbuf[TICKREQLEN];
	char tbuf[2*TICKETLEN];

	if(fsession(fd, trbuf) < 0){
		werrstr("fsession: %r");
		return -1;
	}

	/* no authentication required? */
	memset(tbuf, 0, 2*TICKETLEN);
	if(trbuf[0] == 0)
		return 0;

	/* try getting to an auth server */
	if(afd >= 0)
		return _asgetticket(afd, trbuf, tbuf);
	afd = authdial();
	if(afd < 0){
		werrstr(ccmsg);
		return -1;
	}
	rv = _asgetticket(afd, trbuf, tbuf);
	close(afd);
	if(rv < 0)
		return -1;
	return fauth(fd, tbuf);
}
