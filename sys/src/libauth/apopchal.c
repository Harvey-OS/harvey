#include <u.h>
#include <libc.h>
#include <auth.h>
#include <mp.h>
#include <libsec.h>
#include "authlocal.h"

static char *damsg = "problem with /dev/authenticate";
static char *ccmsg = "can't call AS";
static char *pbmsg = "AS protocol botch";

static int
dochal(Apopchalstate *c, int type)
{
	Ticketreq tr;
	char trbuf[TICKREQLEN];

	c->afd = -1;
	c->asfd = -1;

	/* send request to authentication server and get challenge */
	c->asfd = authdial();
	if(c->asfd < 0){
		werrstr(ccmsg);
		goto err;
	}
	memset(&tr, 0, sizeof(tr));
	tr.type = type;
	convTR2M(&tr, trbuf);
	if(write(c->asfd, trbuf, TICKREQLEN) != TICKREQLEN){
		werrstr(pbmsg);
		goto err;
	}
	if(_asrdresp(c->asfd, c->chal, APOPCHLEN) > 5)
		return 0;
	werrstr(pbmsg);
err:
	if(c->asfd >= 0)
		close(c->asfd);
	c->asfd = -1;
	return -1;
}

static int
doreply(Apopchalstate *c, char *user, char *response, int type)
{
	Ticketreq tr;
	char ticket[TICKETLEN];
	char trbuf[TICKREQLEN];

	/* get ticket request from kernel and send to auth server */
	c->afd = open("/dev/authenticate", ORDWR);
	if(c->afd < 0){
		werrstr(damsg);
		goto err;
	}
	if(read(c->afd, trbuf, TICKREQLEN) != TICKREQLEN){
		werrstr(damsg);
		goto err;
	}
	convM2TR(trbuf, &tr);
	memset(tr.uid, 0, sizeof(tr.uid));
	strcpy(tr.uid, user);
	tr.type = type;
	convTR2M(&tr, trbuf);
	if(write(c->asfd, trbuf, TICKREQLEN) != TICKREQLEN){
		werrstr(pbmsg);
		goto err;
	}

	/* send response to auth server */
	if(strlen(response) != MD5dlen*2){
		werrstr(pbmsg);
		goto err;
	}
	if(write(c->asfd, response, MD5dlen*2) != MD5dlen*2){
		werrstr(pbmsg);
		goto err;
	}
	if(_asrdresp(c->asfd, ticket, TICKETLEN+AUTHENTLEN) < 0){
		/* leave connection open so we can try again */
		close(c->afd);
		c->afd = -1;
		return -2;
	}

	/* pass ticket to /dev/authenticate */
	if(write(c->afd, ticket, TICKETLEN+AUTHENTLEN) != TICKETLEN+AUTHENTLEN){
		goto err;
	}
	close(c->asfd);
	close(c->afd);
	c->afd = c->asfd = -1;
	return 0;
err:
	if(c->afd >= 0)
		close(c->afd);
	if(c->asfd >= 0)
		close(c->asfd);
	c->afd = c->asfd = -1;
	return -1;
}

int
apopchal(Apopchalstate *c)
{
	return dochal(c, AuthApop);
}

int
apopreply(Apopchalstate *c, char *user, char *response)
{
	return doreply(c, user, response, AuthApop);
}

int
cramchal(Cramchalstate *c)
{
	return dochal(c, AuthCram);
}

int
cramreply(Cramchalstate *c, char *user, char *response)
{
	return doreply(c, user, response, AuthCram);
}
