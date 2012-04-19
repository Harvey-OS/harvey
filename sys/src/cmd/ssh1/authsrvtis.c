#include "ssh.h"

static AuthInfo*
authsrvtisfn(Conn *conn, Msg *m)
{
	char *s;
	AuthInfo *ai;
	Chalstate *c;

	free(m);
	if((c = auth_challenge("proto=p9cr user=%q role=server", conn->user)) == nil){
		sshlog("auth_challenge failed for %s", conn->user);
		return nil;
	}
	s = smprint("Challenge: %s\nResponse: ", c->chal);
	if(s == nil){
		auth_freechal(c);
		return nil;
	}
	m = allocmsg(conn, SSH_SMSG_AUTH_TIS_CHALLENGE, 4+strlen(s));
	putstring(m, s);
	sendmsg(m);
	free(s);

	m = recvmsg(conn, 0);
	if(m == nil){
		auth_freechal(c);
		return nil;
	}
	if(m->type != SSH_CMSG_AUTH_TIS_RESPONSE){
		/*
		 * apparently you can just give up on
		 * this protocol and start a new one.
		 */
		unrecvmsg(conn, m);
		return nil;
	}

	c->resp = getstring(m);
	c->nresp = strlen(c->resp);
	ai = auth_response(c);
	auth_freechal(c);
	return ai;
}

Authsrv authsrvtis = 
{
	SSH_AUTH_TIS,
	"tis",
	SSH_CMSG_AUTH_TIS,
	authsrvtisfn,
};
