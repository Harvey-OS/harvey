#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authlocal.h"

static char *damsg = "problem with /dev/authenticate";
static char *ccmsg = "can't call AS";
static char *pbmsg = "AS protocol botch";

int
getchal(Chalstate *c, char *user)
{
	int n;
	Ticketreq tr;
	char trbuf[TICKREQLEN];

	/* get ticket request from kernel and add user name */
	c->afd = open("/dev/authenticate", ORDWR);
	if(c->afd < 0){
		werrstr(damsg);
		return -1;
	}
	n = read(c->afd, trbuf, TICKREQLEN);
	if(n != TICKREQLEN){
		close(c->afd);
		werrstr(damsg);
		return -1;
	}
	convM2TR(trbuf, &tr);
	memset(tr.uid, 0, sizeof(tr.uid));
	strcpy(tr.uid, user);
	tr.type = AuthChal;
	convTR2M(&tr, trbuf);

	/* send request to authentication server and get challenge */
	c->asfd = authdial();
	if(c->asfd < 0){
		werrstr(ccmsg);
		goto err;
	}
	if(write(c->asfd, trbuf, TICKREQLEN) != TICKREQLEN){
		werrstr(pbmsg);
		goto err;
	}
	if(_asrdresp(c->asfd, c->chal, NETCHLEN) < 0)
		goto err;
	return 0;
err:
	close(c->afd);
	close(c->asfd);
	c->afd = c->asfd = -1;
	return -1;
}

int
chalreply(Chalstate *c, char *response)
{
	char resp[NETCHLEN];
	char ticket[TICKETLEN];

	/* send response to auth server and get ticket */
	memset(resp, 0, sizeof resp);
	strncpy(resp, response, NETCHLEN-1);
	if(write(c->asfd, resp, NETCHLEN) != NETCHLEN){
		werrstr(pbmsg);
		goto err;
	}
	if(_asrdresp(c->asfd, ticket, TICKETLEN+AUTHENTLEN) < 0)
		goto err;

	/* pass ticket to /dev/authenticate */
	if(write(c->afd, ticket, TICKETLEN+AUTHENTLEN) != TICKETLEN+AUTHENTLEN){
		werrstr("permission denied");
		goto err;
	}
	close(c->asfd);
	close(c->afd);
	c->afd = c->asfd = -1;
	return 0;
err:
	close(c->asfd);
	close(c->afd);
	c->afd = c->asfd = -1;
	return -1;
}
