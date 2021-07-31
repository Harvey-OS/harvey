#include <u.h>
#include <libc.h>
#include <auth.h>
#include <libsec.h>
#include "authlocal.h"

static char *badreq = "bad ticket request";
static char *ccmsg = "can't connect to AS";
static char *srmsg = "server refused authentication";
static char *sgmsg = "server gave up";

int
authnonce(int fd, uchar *nonce)
{
	int n, afd;
	int rv;
	char trbuf[TICKREQLEN];
	char tbuf[2*TICKETLEN+AUTHENTLEN+TICKETLEN];
	Ticketreq tr;
	Ticket *tp;

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
	if(rv < 0){
		werrstr("getticket: %r");
		return -1;
	}

	/* get authenticator */
	afd = open("/dev/authenticator", ORDWR);
	if(afd < 0){
		werrstr("/dev/authenticator: %r");
		return -1;
	}
	if(write(afd, tbuf, TICKETLEN) < 0){
		close(afd);
		werrstr("writing /dev/authenticator: %r");
		return -1;
	}
	if(read(afd, tbuf+2*TICKETLEN, AUTHENTLEN) < 0){
		close(afd);
		werrstr("reading /dev/authenticator: %r");
		return -1;
	}
	close(afd);

	/* write server ticket to server */
	if(write(fd, tbuf+TICKETLEN, TICKETLEN+AUTHENTLEN) < 0){
		werrstr("%s:%r", srmsg);
		return -1;
	}

	/* get authenticator from server and check */
	if(_asreadn(fd, tbuf+TICKETLEN, AUTHENTLEN) < 0){
		werrstr(sgmsg);
		return -1;
	}
	afd = open("/dev/authcheck", ORDWR);
	if(afd < 0){
		werrstr("authcheck: %r");
		return -1;
	}
	n = write(afd, tbuf, TICKETLEN+AUTHENTLEN);
	if(n < 0){
		close(afd);
		memset(tbuf, 0, AUTHENTLEN);
		if(memcmp(tbuf, tbuf+TICKETLEN, AUTHENTLEN) == 0)
			werrstr("refused by server");
		else
			werrstr("server lies");
		return -1;
	}

	if(nonce != nil){
		read(afd, tbuf, TICKETLEN);
		tp = (Ticket*)tbuf;
		des56to64((uchar*)tp->key, nonce);
	}
	close(afd);

	return 0;
}

int
auth(int fd)
{
	return authnonce(fd, nil);
}
