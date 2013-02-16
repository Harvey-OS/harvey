#include "ssh.h"

static AuthInfo*
authsrvpasswordfn(Conn *c, Msg *m)
{
	char *pass;
	AuthInfo *ai;

	pass = getstring(m);
	ai = auth_userpasswd(c->user, pass);
	free(m);
	return ai;
}

Authsrv authsrvpassword =
{
	SSH_AUTH_PASSWORD,
	"password",
	SSH_CMSG_AUTH_PASSWORD,
	authsrvpasswordfn,
};

