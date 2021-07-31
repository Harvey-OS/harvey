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
	if(!mount(fd, "/mail/ratify", MBEFORE, "")){
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
setupuser(void)
{
	Waitmsg w;
	int pid;

	if(newns(username, 0) < 0)
		bye("user login failed: %r");

	/*
	 * hack to allow access to outgoing smtp forwarding
	 */
	enableForwarding();

	snprint(mboxDir, 3 * NAMELEN, "/mail/box/%s", username);
	if(myChdir(mboxDir) < 0)
		bye("can't open user's mailbox");

	switch(pid = fork()){
	case -1:
		bye("can't initialize mail system");
		break;
	case 0:
		execl("/bin/upas/fs", "upas/fs", "-np", nil);
		_exits(0);
		break;
	default:
		break;
	}
	if(wait(&w) != pid || w.msg[0] != '\0')
		bye("can't initialize mail system");
}

void
initchal(Chalstate *ch)
{
	ch->afd = -1;
	ch->asfd = -1;
}

void
closechal(Chalstate *ch)
{
	if(ch->afd >= 0)
		close(ch->afd);
	if(ch->asfd >= 0)
		close(ch->asfd);
	ch->afd = -1;
	ch->asfd = -1;
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
	Cramchalstate acs;
	char *s, *t;
	int n;

	if(cramchal(&acs) < 0)
		return "couldn't get cram challenge";

	n = strlen(acs.chal);
	s = binalloc(&parseBin, n * 2, 0);
	n = enc64(s, n * 2, (uchar*)acs.chal, n);
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
	strncpy(username, s, NAMELEN);
	username[NAMELEN-1] = '\0';

	if(cramreply(&acs, username, t) < 0)
		return "login failed";

	setupuser();

	return nil;
}

static char*
cramLogin(char *user, char *secret)
{
	Cramchalstate acs;
	uchar digest[MD5dlen];
	char response[2*MD5dlen+1];
	int i;

	if(cramchal(&acs) < 0)
		return "couldn't get cram challenge";

	hmac_md5((uchar*)acs.chal, strlen(acs.chal),
		(uchar*)secret, strlen(secret), digest,
		nil);
	for(i = 0; i < MD5dlen; i++)
		snprint(response + 2*i, sizeof(response) - 2*i, "%2.2ux", digest[i]);

	if(cramreply(&acs, user, response) < 0)
		return "login failed";

	return nil;
}

int
passCheck(char *user, char *pass)
{
	return cramLogin(user, pass) == nil;
}


#define USER		"VXNlcg=="
#define PASSWORD	"UGFzc3dvcmQ="

char*
loginauth(void)
{
	Chalstate ch;
	char *s, *u;

	Bprint(&bout, "+ %s\r\n", USER);
	if(Bflush(&bout) < 0)
		writeErr();

	u = authresp();
	if(u == nil || getchal(&ch, u) < 0)
		return "login failed";
	Bprint(&bout, "* ok [ALERT] encrypt challenge, %s, as a password\r\n", ch.chal);
	Bprint(&bout, "+ %s\r\n", PASSWORD);
	if(Bflush(&bout) < 0)
		writeErr();

	s = authresp();
	if(s == nil || chalreply(&ch, s) < 0)
		return "login failed";

	strncpy(username, u, NAMELEN);
	username[NAMELEN-1] = '\0';

	setupuser();
	return nil;
}
