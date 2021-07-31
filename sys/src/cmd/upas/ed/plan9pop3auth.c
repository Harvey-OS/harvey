#include "common.h"
#include "print.h"
#include "pop3.h"
#include <auth.h>
#include <libsec.h>

Apopchalstate chs;
extern int passwordinclear;

extern void
hello(void)
{
	if(apopchal(&chs) < 0)
		senderr("auth server not responding, try later");

	sendok("POP3 server ready %s", chs.chal);
}

extern int
usercmd(char *arg)
{
	if(loggedin)
		return senderr("already authenticated");
	if(*arg == 0)
		return senderr("USER requires argument");
	strncpy(user, arg, sizeof(user));
	user[NAMELEN-1] = 0;
	return sendok("");
}

static int
dologin(char *user, char *box, char *response)
{
	int rv;
	static int tries;

	syslog(0, "pop3", "user %s login", user);
	rv = apopreply(&chs, user, response);
	syslog(0, "pop3", "user %s response %d %s", user, rv, response);
	switch(rv){
	case 0:
		loggedin = 1;
		newns(user, 0);
		mailfile = s_new();
		mboxname(box, mailfile);
		if(P(s_to_c(mailfile))){
			senderr("file busy");
			exits(0);
		}
		read_mbox(s_to_c(mailfile), reverse);
		return sendok("");
	case -2:
		if(tries++ < 5)
			return senderr("authentication failed");
		break;
	}
	senderr("authentication failed %r, server exiting");
	exits(0);

	/* unreachable... */
	return 0;
}

extern int
passcmd(char *arg)
{
	DigestState *s;
	uchar digest[MD5dlen];
	char response[2*MD5dlen];
	int i;

	if(passwordinclear == 0)
		senderr("password in the clear disallowed");

	/* use password to encode challenge */
	if(apopchal(&chs) < 0)
		senderr("couldn't get apop challenge");

	// hash challenge with secret and convert to ascii
	s = md5((uchar*)chs.chal, strlen(chs.chal), 0, 0);
	md5((uchar*)arg, strlen(arg), digest, s);
	for(i = 0; i < MD5dlen; i++)
		sprint(response + 2*i, "%2.2ux", digest[i]);

	return dologin(user, user, response);
}

extern int
apopcmd(char *arg)
{
	char *resp, *p;
	char box[3*NAMELEN];

	resp = nextarg(arg);
	strncpy(box, arg, sizeof(box));
	box[sizeof(box)-1] = 0;
	strncpy(user, box, sizeof(user));
	user[NAMELEN-1] = 0;
	if(p = strchr(user, '/'))
		*p = '\0';
	return dologin(user, box, resp);
}

