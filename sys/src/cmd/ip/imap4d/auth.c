#include <u.h>
#include <libc.h>
#include <auth.h>
#include <libsec.h>
#include <bio.h>
#include "imap4d.h"

/*
 * hack to allow smtp forwarding.
 * hide the peer IP address under a rock in the ratifier FS.
 */
void
enableForwarding(void)
{
	char buf[64], peer[64], *p;
	static ulong last;
	ulong now;
	int fd;

	if(remote == nil)
		return;

	now = time(0);
	if(now < last + 5*60)
		return;
	last = now;

	fd = open("/srv/ratify", ORDWR);
	if(fd < 0)
		return;
	if(!mount(fd, -1, "/mail/ratify", MBEFORE, "")){
		close(fd);
		return;
	}
	close(fd);

	strncpy(peer, remote, sizeof(peer));
	peer[sizeof(peer) - 1] = '\0';
	p = strchr(peer, '!');
	if(p != nil)
		*p = '\0';

	snprint(buf, sizeof(buf), "/mail/ratify/trusted/%s#32", peer);

	/*
	 * if the address is already there and the user owns it,
	 * remove it and recreate it to give him a new time quanta.
	 */
	if(access(buf, 0) >= 0 && remove(buf) < 0)
		return;

	fd = create(buf, OREAD, 0666);
	if(fd >= 0)
		close(fd);
}

void
setupuser(AuthInfo *ai)
{
	Waitmsg *w;
	int pid;

	if(ai){
		strecpy(username, username+sizeof username, ai->cuid);

		if(auth_chuid(ai, nil) < 0)
			bye("user auth failed: %r");
		auth_freeAI(ai);
	}else
		strecpy(username, username+sizeof username, getuser());

	if(newns(username, 0) < 0)
		bye("user login failed: %r");

	/*
	 * hack to allow access to outgoing smtp forwarding
	 */
	enableForwarding();

	snprint(mboxDir, MboxNameLen, "/mail/box/%s", username);
	if(myChdir(mboxDir) < 0)
		bye("can't open user's mailbox");

	switch(pid = fork()){
	case -1:
		bye("can't initialize mail system");
		break;
	case 0:
		execl("/bin/upas/fs", "upas/fs", "-np", nil);
_exits("rob1");
		_exits(0);
		break;
	default:
		break;
	}
	if((w=wait()) == nil || w->pid != pid || w->msg[0] != '\0')
		bye("can't initialize mail system");
	free(w);
}

static char*
authresp(void)
{
	char *s, *t;
	int n;

	t = Brdline(&bin, '\n');
	n = Blinelen(&bin);
	if(n < 2)
		return nil;
	n--;
	if(t[n-1] == '\r')
		n--;
	t[n] = '\0';
	if(n == 0 || strcmp(t, "*") == 0)
		return nil;

	s = binalloc(&parseBin, n + 1, 0);
	n = dec64((uchar*)s, n, t, n);
	s[n] = '\0';
	return s;
}

/*
 * rfc 2195 cram-md5 authentication
 */
char*
cramauth(void)
{
	AuthInfo *ai;
	Chalstate *cs;
	char *s, *t;
	int n;

	if((cs = auth_challenge("proto=cram role=server")) == nil)
		return "couldn't get cram challenge";

	n = cs->nchal;
	s = binalloc(&parseBin, n * 2, 0);
	n = enc64(s, n * 2, (uchar*)cs->chal, n);
	Bprint(&bout, "+ ");
	Bwrite(&bout, s, n);
	Bprint(&bout, "\r\n");
	if(Bflush(&bout) < 0)
		writeErr();

	s = authresp();
	if(s == nil)
		return "client cancelled authentication";

	t = strchr(s, ' ');
	if(t == nil)
		bye("bad auth response");
	*t++ = '\0';
	strncpy(username, s, UserNameLen);
	username[UserNameLen-1] = '\0';

	cs->user = username;
	cs->resp = t;
	cs->nresp = strlen(t);
	if((ai = auth_response(cs)) == nil)
		return "login failed";
	auth_freechal(cs);
	setupuser(ai);
	return nil;
}

AuthInfo*
passLogin(char *user, char *secret)
{
	AuthInfo *ai;
	Chalstate *cs;
	uchar digest[MD5dlen];
	char response[2*MD5dlen+1];
	int i;

	if((cs = auth_challenge("proto=cram role=server")) == nil)
		return nil;

	hmac_md5((uchar*)cs->chal, strlen(cs->chal),
		(uchar*)secret, strlen(secret), digest,
		nil);
	for(i = 0; i < MD5dlen; i++)
		snprint(response + 2*i, sizeof(response) - 2*i, "%2.2ux", digest[i]);

	cs->user = user;
	cs->resp = response;
	cs->nresp = strlen(response);
	ai = auth_response(cs);
	auth_freechal(cs);
	return ai;
}
