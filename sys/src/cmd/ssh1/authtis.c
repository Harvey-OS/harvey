#include "ssh.h"

static int
authtisfn(Conn *c)
{
	int fd, n;
	char *chal, resp[256];
	Msg *m;

	if(!c->interactive)
		return -1;

	debug(DBG_AUTH, "try TIS\n");
	sendmsg(allocmsg(c, SSH_CMSG_AUTH_TIS, 0));

	m = recvmsg(c, -1);
	switch(m->type){
	default:
		badmsg(m, SSH_SMSG_AUTH_TIS_CHALLENGE);
	case SSH_SMSG_FAILURE:
		free(m);
		return -1;
	case SSH_SMSG_AUTH_TIS_CHALLENGE:
		break;
	}

	chal = getstring(m);
	free(m);

	if((fd = open("/dev/cons", ORDWR)) < 0)
		error("can't open console");

	fprint(fd, "TIS Authentication\n%s", chal);
	n = read(fd, resp, sizeof resp-1);
	if(n < 0)
		resp[0] = '\0';
	else
		resp[n] = '\0';

	if(resp[0] == 0 || resp[0] == '\n')
		return -1;

	m = allocmsg(c, SSH_CMSG_AUTH_TIS_RESPONSE, 4+strlen(resp));
	putstring(m, resp);
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

Auth authtis =
{
	SSH_AUTH_TIS,
	"tis",
	authtisfn,
};
