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

	/* get a ticket from an auth server */
	if(afd < 0){
		afd = authdial();
		if(afd < 0){
			werrstr(ccmsg);
			return -1;
		}
		rv = _asgetticket(afd, trbuf, tbuf);
		close(afd);
	} else
		rv = _asgetticket(afd, trbuf, tbuf);
	if(rv < 0)
		return -1;

	/* pass ticket to kernel */
	return fauth(fd, tbuf);
}
