#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authlocal.h"

static char *abmsg = "/dev/authenticate: %r";
static char *badreq = "bad ticket request";
static char *ccmsg = "can't connect to AS";

int
login(char *user, char *password, char *namespace)
{
	int fd, afd, rv;
	char trbuf[TICKREQLEN];
	char tbuf[2*TICKETLEN+AUTHENTLEN];
	char key[DESKEYLEN];
	Ticketreq tr;
	Ticket t;
	Authenticator a;

	/* get ticket request from kernel */
	afd = open("/dev/authenticate", ORDWR);
	if(afd < 0){
		werrstr(abmsg);
		return -1;
	}
	rv = read(afd, trbuf, TICKREQLEN);
	if(rv != TICKREQLEN){
		close(afd);
		werrstr(abmsg);
		return -1;
	}

	/* add in user and host id */
	convM2TR(trbuf, &tr);
	if(tr.type != AuthTreq){
		werrstr(badreq);
		return -1;
	}
	memset(tr.uid, 0, sizeof(tr.uid));
	strcpy(tr.uid, user);
	memset(tr.hostid, 0, sizeof(tr.hostid));
	strcpy(tr.hostid, user);
	convTR2M(&tr, trbuf);

	/* pass to auth server for ticket ticket */
	fd = authdial();
	if(fd < 0){
		close(afd);
		werrstr(ccmsg);
		return -1;
	}
	rv = _asgetticket(fd, trbuf, tbuf);
	close(fd);
	if(rv < 0){
		close(afd);
		werrstr("getticket: %r");
		return -1;
	}

	/* crack ticket for nonce key */
	passtokey(key, password);
	convM2T(tbuf, &t, key);
	if(t.num != AuthTc){
		close(afd);
		werrstr("login failed");
		return -1;
	}
		
	/* create an authenticator and pass to kernel */
	a.num = AuthAc;
	memmove(a.chal, t.chal, CHALLEN);
	a.id = 0;
	convA2M(&a, tbuf+2*TICKETLEN, t.key);
	rv = write(afd, tbuf+TICKETLEN, TICKETLEN+AUTHENTLEN);
	close(afd);
	if(rv < 0)
		return -1;

	/* set up namespace */
	newns(user, namespace);
	return 0;
}
