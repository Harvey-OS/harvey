#include "lib9.h"
#include "auth.h"
#include "authlocal.h"

static char *badreq = "bad ticket request";
static char *ccmsg = "can't connect to AS";
static char *srmsg = "server refused authentication";
static char *sgmsg = "server gave up";

int
auth(int fd)
{
	int n, afd;
	int rv;
	char trbuf[TICKREQLEN];
	char tbuf[2*TICKETLEN+AUTHENTLEN];
	char ebuf[ERRLEN];
	Ticketreq tr;

	ebuf[0] = 0;
	errstr(ebuf);
	/* add uid and local hostid to ticket request */
	if(_asreadn(fd, trbuf, TICKREQLEN) < 0){
		werrstr(badreq);
		return -1;
	}
	convM2TR(trbuf, &tr);
	if(tr.type != AuthTreq){
		werrstr(badreq);
		return -1;
	}
	memset(tr.uid, 0, sizeof(tr.uid));
	strcpy(tr.uid, getuser());
	memset(tr.hostid, 0, sizeof(tr.hostid));
	_asrdfile("/dev/hostowner", tr.hostid, NAMELEN);
	convTR2M(&tr, trbuf);

	/* get ticket */
	afd = authdial();
	if(afd < 0){
		werrstr(ccmsg);
		return -1;
	}
	rv = _asgetticket(afd, trbuf, tbuf);
	close(afd);
	if(rv)
		return -1;

	ebuf[0] = 0;
	errstr(ebuf);
	/* get authenticator */
	afd = open("/dev/authenticator", ORDWR);
	if(afd < 0){
		werrstr("/dev/authenticator: %r");
		return -1;
	}
	ebuf[0] = 0;
	errstr(ebuf);
	if(write(afd, tbuf, TICKETLEN) < 0){
		werrstr("writing /dev/authenticator: %r");
		return -1;
	}
	ebuf[0] = 0;
	errstr(ebuf);
	if(read(afd, tbuf+2*TICKETLEN, AUTHENTLEN) < 0){
		werrstr("reading /dev/authenticator: %r");
		return -1;
	}

	ebuf[0] = 0;
	errstr(ebuf);
	/* write server ticket to server */
	if(write(fd, tbuf+TICKETLEN, TICKETLEN+AUTHENTLEN) < 0){
		werrstr("%s:%r", srmsg);
		return -1;
	}

	ebuf[0] = 0;
	errstr(ebuf);
	/* get authenticator from server and check */
	if(_asreadn(fd, tbuf+TICKETLEN, AUTHENTLEN) < 0){
		werrstr(sgmsg);
		return -1;
	}
	ebuf[0] = 0;
	errstr(ebuf);
	afd = open("/dev/authcheck", ORDWR);
	if(afd < 0){
		werrstr("authcheck: %r");
		return -1;
	}
	n = write(afd, tbuf, TICKETLEN+AUTHENTLEN);
	close(afd);
	if(n < 0){
		memset(tbuf, 0, AUTHENTLEN);
		if(memcmp(tbuf, tbuf+TICKETLEN, AUTHENTLEN) == 0)
			werrstr("refused by server");
		else
			werrstr("server lies");
		return -1;
	}
	return 0;
}
