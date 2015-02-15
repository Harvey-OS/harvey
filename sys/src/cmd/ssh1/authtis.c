/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
