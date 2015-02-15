/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <plan9.h>
#include <sys/socket.h>	/* various networking crud */
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

void
getremotehostname(char *name, int nname)
{
	struct sockaddr_in sock;
	struct hostent *hp;
	uint len;
	int on;

	strecpy(name, name+nname, "unknown");
	len = sizeof sock;
	if(getpeername(0, (struct sockaddr*)&sock, (void*)&len) < 0)
		return;

	hp = gethostbyaddr((char *)&sock.sin_addr, sizeof (struct in_addr),
		sock.sin_family);
	if(hp == 0)
		return;

	strecpy(name, name+nname, hp->h_name);
	on = 1;
	setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, (char*)&on, sizeof(on));
#ifdef TCP_NODELAY
	on = 1;
	setsockopt(0, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on));
#endif
}
