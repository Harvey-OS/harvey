#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

/*
 * A->C: ChA
 * C->A: C, KC{CKcchal, ChA, oPC, nPC}
 * A->C: "password changed"
 * or
 * A->C: E
 */

char *passwd(char*, char*);

void
main(int argc, char *argv[])
{
	char chal[AUTHLEN], req[NAMELEN+AUTHLEN+2*PASSLEN], *m, rep[ERRLEN];
	int i;

	USED(argc, argv);
	argv0 = "changekey";

	srand(time(0) * getpid());
	for(i=1; i<AUTHLEN; i++)
		chal[i] = nrand(256);
	write(1, chal+1, AUTHLEN-1);
	chal[0] = CKcchal;
	if(read(0, req, sizeof req) != sizeof req)
		exits(0);
	m = passwd(chal, req);
	strncpy(rep, m, ERRLEN);
	if(write(1, rep, sizeof rep) != sizeof rep)
		syslog(0, AUTHLOG, "error: %r replying to user");
	exits(0);
}

char *
passwd(char *chal, char *req)
{
	char *user, *key, buf[sizeof KEYDB+NAMELEN+6];
	char ok[DESKEYLEN], nk[DESKEYLEN], opass[PASSLEN+1], npass[PASSLEN+1];
	int fd;

	user = req;
	user[NAMELEN-1] = '\0';
	req += NAMELEN;
	key = findkey(user);
	if(!key){
		syslog(0, AUTHLOG, "user not found");
		return "user unknown";
	}
	decrypt(key, req, AUTHLEN+2*PASSLEN);
	if(memcmp(chal, req, AUTHLEN) != 0){
		syslog(0, AUTHLOG, "user %s authentication failure", user);
		logfail(user);
		return "authentication failure";
	}
	memmove(opass, req+AUTHLEN, PASSLEN);
	opass[PASSLEN] = '\0';
	memmove(npass, req+AUTHLEN+PASSLEN, PASSLEN);
	npass[PASSLEN] = '\0';
	passtokey(ok, opass);
	if(memcmp(ok, key, DESKEYLEN) != 0){
		syslog(0, AUTHLOG, "user %s old password mismatch", user);
		logfail(user);
		return "authentication failure";
	}
	if(!passtokey(nk, npass) || !okpasswd(npass)){
		syslog(0, AUTHLOG, "bad password for %s", user);
		return "bad password";
	}
	sprint(buf, "%s/%s/key", KEYDB, user);
	fd = open(buf, OWRITE);
	if(fd < 0 || write(fd, nk, DESKEYLEN) != DESKEYLEN){
		syslog(0, AUTHLOG, "can't set password for %s", user);
		return "can't set password";
	}
	close(fd);
	syslog(0, AUTHLOG, "password for %s changed", user);
	succeed(user);
	return "password changed";
}
