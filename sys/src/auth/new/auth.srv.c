#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

enum{
	Rauthlen= 2*AUTHLEN + 3*NAMELEN + DOMLEN,
	Wauthlen= ERRLEN + 1,
};

char	*checkuser(char*, char*);

void
main(int argc, char *argv[])
{
	char r[Rauthlen], w[Wauthlen];
	char buf[8192];
	int n;

	USED(argc, argv);
	argv0 = "auth";

	for(;;){
		n = read(0, r, Rauthlen);
		memset(w, 0, sizeof w);
		switch(n){
		case Rauthlen:
			if(err = checkuser(r, w)){
				w[0] = Aerr;
				strncpy(&w[1], err, ERRLEN);
			}
			if(write(1, w, Wauthlen) != Wauthlen){
				syslog(0, AUTHLOG, "error: %r replying to %s", server);
				exits("err");
			}
			break;
		case 0:
			exits("eof");
		case -1:
			syslog(0, AUTHLOG, "error: %r from %s", server);
			exits("err");
		default:
			syslog(0, AUTHLOG, "bad read count %d from %s", n, server);
			exits("err");
		}
	}
}

/*
 * S->A	KH{Aschal, ChH, U, D, KC{Acchal, ChU, UH}}, H
 * A->S	KC{Actick, ChU, KN, KH{Astick, ChH, U', KN}}
 * or
 * A->S Aerr, ERR
 */
char*
checkuser(char *req, char *resp)
{
	char *domain, *user, *auser, *ukey, *uchal, *uhost;
	char *host, *hkey, *hchal;
	char noncekey[DESKEYLEN];
	int i;

	/*
	 * set up pointers to message parts
	 */
	hchal = req;
	user = hchal + AUTHELEN;
	domain = user + NAMELEN;
	uchal = domain + DOMLEN;
	uhost = uchal + AUTHLEN;
	host = uhost + NAMELEN;

	host[NAMELEN-1] = '\0';

	hkey = findkey(host);
	if(!hkey || !ishost(host)){
		syslog(0, AUTHLOG, "server %s unknown", server);
		return "server unknown";
	}
	decrypt(hkey, req, 2*AUTHLEN + 3*NAMELEN);
	if(hchal[0] != Aschal){
		syslog(0, AUTHLOG, "server '%s': bad challenge number", host);
		return "server failed authentication";
	}

	user[NAMELEN-1] = '\0';
	domain[DOMLEN-1] = '\0';
	auser = mapuser(user, domain);
	ukey = findkey(auser);
	if(!ukey){
		syslog(0, AUTHLOG, "user unknown", server);
		return "user unknown";
	}
	decrypt(ukey, req, AUTHLEN);
	uhost[NAMELEN-1] = '\0';
	if(uchal[0] != Acchal){
		syslog(0, AUTHLOG, "user '%s': bad challenge number", user);
		return "user failed authentication";
	}
	if(strcmp(uhost, host) != 0 && strcmp(uhost, "any") != 0){
		syslog(0, AUTHLOG, "host name mismatch %s %s", host, uhost);
		return "host names do not match";
	}

	strrnand(noncekey, DESKEYLEN);
	i = 0;
	memmove(&resp[i], hchal, AUTHLEN);
	resp[i] = Astick;
	i += AUTHLEN;
	strncpy(&resp[i], user, NAMELEN);
	i += NAMELEN;
	memove(&resp[i], noncekey, DESKEYLEN);
	i += DESKEYLEN;
	encrypt(hkey, resp, i);

	memmove(&resp[i], uchal, AUTHLEN);
	resp[i] = Actick;
	i += AUTHLEN;
	memove(&resp[i], noncekey, DESKEYLEN);
	i += DESKEYLEN;
	encrypt(ukey, resp, i);

	syslog(0, AUTHLOG, "user %s to %s ok", user, host);
	succeed(user);
	return 0;
}
