#include <alef.h>
#include <auth.h>
#include "authlocal.h"

intern byte *damsg = "problem with /dev/authenticate";
intern byte *ccmsg = "can't call AS";
intern byte *pbmsg = "AS protocol botch";

int
getchal(Chalstate *c, byte *user)
{
	int n;
	Ticketreq tr;
	byte trbuf[TICKREQLEN];

	rescue {
		werrstr(damsg);
		return -1;
	}

	/* get ticket request from kernel and add user name */
	c->afd = open("/dev/authenticate", ORDWR);
	if(c->afd < 0)
		raise;

	n = read(c->afd, trbuf, TICKREQLEN);
	if(n != TICKREQLEN){
		close(c->afd);
		raise;
	}
	convM2TR(trbuf, &tr);
	memset(tr.uid, 0, sizeof(tr.uid));
	strcpy(tr.uid, user);
	tr.type = AuthChal;
	convTR2M(&tr, trbuf);

	rescue {
		close(c->afd);
		close(c->asfd);
		c->afd = c->asfd = -1;
		return -1;
	}

	/* send request to authentication server and get challenge */
	c->asfd = authdial();
	if(c->asfd < 0){
		werrstr(ccmsg);
		raise;
	}
	if(write(c->asfd, trbuf, TICKREQLEN) != TICKREQLEN){
		werrstr(pbmsg);
		raise;
	}
	if(_asrdresp(c->asfd, c->chal, NETCHLEN) < 0)
		raise;
	return 0;
}

int
chalreply(Chalstate *c, byte *response)
{
	byte resp[NETCHLEN];
	byte ticket[TICKETLEN];

	rescue {
		close(c->asfd);
		close(c->afd);
		c->afd = c->asfd = -1;
		return -1;
	}

	/* send response to auth server and get ticket */
	memset(resp, 0, sizeof resp);
	strncpy(resp, response, NETCHLEN-1);
	if(write(c->asfd, resp, NETCHLEN) != NETCHLEN){
		werrstr(pbmsg);
		raise;
	}
	if(_asrdresp(c->asfd, ticket, TICKETLEN) < 0)
		raise;

	/* pass ticket to /dev/authenticate */
	if(write(c->afd, ticket, TICKETLEN) != TICKETLEN){
		werrstr("permission denied");
		raise;
	}
	close(c->asfd);
	close(c->afd);
	c->afd = c->asfd = -1;
	return 0;
}
