#include <u.h>
#include <libc.h>
#include <auth.h>
#include <mp.h>
#include <libsec.h>
#include "authlocal.h"

static char *damsg = "problem with /dev/authenticate";
static char *ccmsg = "can't call AS";
static char *pbmsg = "AS protocol botch";

int
vncchal(VNCchalstate *c, char *user)
{
	int n;
	Ticketreq tr;
	char trbuf[TICKREQLEN];

	/* get ticket request from kernel and add user name */
	c->asfd = -1;
	c->afd = open("/dev/authenticate", ORDWR);
	if(c->afd < 0){
		werrstr(damsg);
		goto err;
	}
	n = read(c->afd, trbuf, TICKREQLEN);
	if(n != TICKREQLEN){
		werrstr(damsg);
		goto err;
	}
	convM2TR(trbuf, &tr);
	memset(tr.uid, 0, sizeof(tr.uid));
	strcpy(tr.uid, user);
	tr.type = AuthVNC;
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
	c->challen = _asrdresp(c->asfd, (char*)c->chal, sizeof(c->chal));
	if(c->challen < 0)
		goto err;
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
vncreply(VNCchalstate *c, uchar *response)
{
	char ticket[TICKETLEN+AUTHENTLEN];

	/* send response to auth server and get ticket */
	if(write(c->asfd, response, c->challen) != c->challen){
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
	if(c->afd >= 0)
		close(c->afd);
	if(c->asfd >= 0)
		close(c->asfd);
	c->afd = c->asfd = -1;
	return -1;
}
