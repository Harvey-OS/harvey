#include "ssh.h"

static int
authpasswordfn(Conn *c)
{
	Msg *m;
	UserPasswd *up;

	up = auth_getuserpasswd(c->interactive ? auth_getkey : nil, "proto=pass service=ssh server=%q user=%q", c->host, c->user);
	if(up == nil){
		debug(DBG_AUTH, "getuserpasswd returned nothing (interactive=%d)\n", c->interactive);
		return -1;
	}

	debug(DBG_AUTH, "try using password from factotum\n");
	m = allocmsg(c, SSH_CMSG_AUTH_PASSWORD, 4+strlen(up->passwd));
	putstring(m, up->passwd);
	sendmsg(m);

	m = recvmsg(c, -1);
	switch(m->type){
	default:
		badmsg(m, 0);
	case SSH_SMSG_SUCCESS:
		free(m);
		return 0;
	case SSH_SMSG_FAILURE:
		free(m);
		return -1;
	}
}

Auth authpassword =
{
	SSH_AUTH_PASSWORD,
	"password",
	authpasswordfn,
};
