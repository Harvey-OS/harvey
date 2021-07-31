#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

void	checkuser(char*, char*);

/*
 * c -> a	KC{KH{RXschal, hchal}, host, RXcchal, cchal}, client
 * a -> c	KC{KH{RXstick, hchal, client, KC}, RXctick, cchal}
 */

void
main(int argc, char *argv[])
{
	char req[2*AUTHLEN+2*NAMELEN], resp[2*AUTHLEN+NAMELEN+DESKEYLEN];

	USED(argc, argv);
	argv0 = "rexauth";
	if(read(0, req, sizeof req) != sizeof req)
		fail(0);
	checkuser(req, resp);
	if(write(1, resp, sizeof resp) != sizeof resp)
		syslog(0, AUTHLOG, "error: %r replying to server");
	exits(0);
}

void
checkuser(char *req, char *resp)
{
	char *user, *ukey, *host, *hkey;
	int i;

	user = req+AUTHLEN+NAMELEN+AUTHLEN;
	user[NAMELEN-1] = '\0';
	ukey = findkey(user);
	if(!ukey){
		syslog(0, AUTHLOG, "user unknown");
		fail(0);
	}
	decrypt(ukey, req, AUTHLEN+NAMELEN+AUTHLEN);
	host = req+AUTHLEN;
	host[NAMELEN-1] = '\0';
	hkey = findkey(host);
	if(!hkey || !ishost(host)){
		syslog(0, AUTHLOG, "user %s: server unknown", user);
		fail(0);
	}
	decrypt(hkey, req, AUTHLEN);
	if(req[0] != RXschal){
		syslog(0, AUTHLOG, "server '%s': bad challenge number", host);
		fail(0);
	}
	if(req[AUTHLEN+NAMELEN] != RXcchal){
		syslog(0, AUTHLOG, "user '%s': bad challenge number", user);
		fail(0);
	}

	i = 0;
	memmove(&resp[i], req, AUTHLEN);
	resp[i] = RXstick;
	i += AUTHLEN;
	strncpy(&resp[i], user, NAMELEN);
	i += NAMELEN;
	memmove(&resp[i], ukey, DESKEYLEN);
	i += DESKEYLEN;
	encrypt(hkey, resp, i);

	memmove(&resp[i], req+AUTHLEN+NAMELEN, AUTHLEN);
	resp[i] = RXctick;
	i += AUTHLEN;
	encrypt(ukey, resp, i);
	syslog(0, AUTHLOG, "user %s to %s ok", user, host);
	succeed(user);
}
