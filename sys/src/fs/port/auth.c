#include	"all.h"
#include	"io.h"

#include	<authsrv.h>

Nvrsafe	nvr;

char*
nvrgetconfig(void)
{
	return nvr.config;
}

int
nvrsetconfig(char* word)
{
	int c;

	c = strlen(word);
	if(c >= sizeof(nvr.config)) {
		print("config string too long\n");
		return 1;
	}
	memset(nvr.config, 0, sizeof(nvr.config));
	memmove(nvr.config, word, c);
	nvr.configsum = nvcsum(nvr.config, sizeof(nvr.config));
	nvwrite(NVRAUTHADDR, &nvr, sizeof(nvr));

	return 0;
}

int
nvrcheck(void)
{
	uchar csum;

	print("nvr read\n");
	nvread(NVRAUTHADDR, &nvr, sizeof(nvr));

	csum = nvcsum(nvr.authkey, sizeof(nvr.authkey));
	if(csum != nvr.authsum) {
		print("\n\n ** NVR key checksum is incorrect  **\n");
		print(" ** set password to allow attaches **\n\n");
		memset(nvr.authkey, 0, sizeof(nvr.authkey));
		return 1;
	}

	csum = nvcsum(nvr.config, sizeof(nvr.config));
	if(csum != nvr.configsum) {
		print("\n\n ** NVR config checksum is incorrect  **\n");
		memset(nvr.config, 0, sizeof(nvr.config));
		return 1;
	}

	return 0;
}

void
cmd_passwd(int, char *[])
{
	char passwd[32];
	static char zeros[DESKEYLEN];
	char nkey1[DESKEYLEN], nkey2[DESKEYLEN];
	char authid[NAMELEN];
	char authdom[DOMLEN];

	if(memcmp(nvr.authkey, zeros, sizeof(nvr.authkey))) {
		print("Old password:");
		getstring(passwd, sizeof(passwd), 0);
		memset(nkey1, 0, DESKEYLEN);
		passtokey(nkey1, passwd);
		if(memcmp(nkey1, nvr.authkey, DESKEYLEN)) {
			print("Bad password\n");
			delay(1000);
			return;
		}
	}

	print("New password:");
	getstring(passwd, sizeof(passwd), 0);
	memset(nkey1, 0, DESKEYLEN);
	passtokey(nkey1, passwd);

	print("Confirm password:");
	getstring(passwd, sizeof(passwd), 0);
	memset(nkey2, 0, DESKEYLEN);
	passtokey(nkey2, passwd);

	if(memcmp(nkey1, nkey2, DESKEYLEN)) {
		print("don't match\n");
		return;
	}
	memmove(nvr.authkey, nkey1, DESKEYLEN);
	nvr.authsum = nvcsum(nvr.authkey, DESKEYLEN);

	print("Authentication id:");
	getstring(authid, sizeof(authid), 1);
	if(authid[0]){
		memset(nvr.authid, 0, NAMELEN);
		strcpy(nvr.authid, authid);
		nvr.authidsum = nvcsum(nvr.authid, NAMELEN);
	}

	print("Authentication domain:");
	getstring(authdom, sizeof(authdom), 1);
	if(authdom[0]){
		memset(nvr.authdom, 0, NAMELEN);
		strcpy(nvr.authdom, authdom);
		nvr.authdomsum = nvcsum(nvr.authdom, NAMELEN);
	}
	nvwrite(NVRAUTHADDR, &nvr, sizeof(nvr));
}

void
getstring(char *str, int n, int doecho)
{
	int c;
	char *p, *e;

	memset(str, 0, n);
	p = str;
	e = str+n-1;
	echo = doecho;
	for(;;) {
		if(p == e) {
			*p = '\0';
			goto out;
		}
		c = getc();
		switch(c) {
		case '\n':
			*p = '\0';
			print("\n");
			goto out;
		case '\b':
			if(p > str)
				p--;
			break;
		case 'U' - '@':
			p = str;
			break;
		default:
			*p++ = c;
		}
	}
out:
	echo = 1;
}

int
conslock(void)
{
	static char zeroes[DESKEYLEN];
	char passwd[128];
	char nkey1[DESKEYLEN];

	if(memcmp(nvr.authkey, zeroes, DESKEYLEN) == 0) {
		print("no password set\n");
		return 0;
	}

	for(;;) {
		print("%s password:", service);
		getstring(passwd, sizeof(passwd), 0);
		memset(nkey1, 0, DESKEYLEN);
		passtokey(nkey1, passwd);
		if(memcmp(nkey1, nvr.authkey, DESKEYLEN) == 0) {
			prdate();
			return 1;
		}

		print("Bad password\n");
		delay(1000);
	}
	return 0;
}

/*
 *  authentication specific to 9P2000
 */

/* authentication states */
enum
{
	HaveProtos=1,
	NeedProto,
	HaveOK,
	NeedCchal,
	HaveSinfo,
	NeedTicket,
	HaveSauthenticator,
	SSuccess,
};

char *phasename[] =
{
[HaveProtos]	"HaveProtos",
[NeedProto]	"NeedProto",
[HaveOK]	"HaveOK",
[NeedCchal]	"NeedCchal",
[HaveSinfo]	"HaveSinfo",
[NeedTicket]	"NeedTicket",
[HaveSauthenticator]	"HaveSauthenticator",
[SSuccess]	"SSuccess",
};

/* authentication structure */
struct	Auth
{
	int	inuse;
	char	uname[NAMELEN];	/* requestor's remote user name */
	char	aname[NAMELEN];	/* requested aname */
	short	uid;		/* uid decided on */
	int	phase;
	char	cchal[CHALLEN];
	char	tbuf[TICKETLEN+AUTHENTLEN];	/* server ticket */
	Ticket	t;
	Ticketreq tr;
};

Auth*	auths;
Lock	authlock;

void
authinit(void)
{
	auths = ialloc(conf.nauth * sizeof(*auths), 0);
}

static int
failure(Auth *s, char *why)
{
	int i;

if(*why)print("authentication failed: %s: %s\n", phasename[s->phase], why);
	srand((ulong)s + m->ticks);
	for(i = 0; i < CHALLEN; i++)
		s->tr.chal[i] = nrand(256);
	s->uid = -1;
	strncpy(s->tr.authid, nvr.authid, NAMELEN);
	strncpy(s->tr.authdom, nvr.authdom, DOMLEN);
	memmove(s->cchal, s->tr.chal, sizeof(s->cchal));
	s->phase = HaveProtos;
	return -1;
}

Auth*
authnew(char *uname, char *aname)
{
	static int si = 0;
	int i;
	Auth *s;

	i = si + 1;
	if(i < 0 || i >= conf.nauth)
		i = 0;
	si = i;
	for(;;){
		s = &auths[i++];
		if(s->inuse)
			continue;
		lock(&authlock);
		if(s->inuse == 0){
			s->inuse = 1;
			strncpy(s->uname, uname, NAMELEN-1);
			strncpy(s->aname, aname, NAMELEN-1);
			failure(s, "");
			si = i;
			unlock(&authlock);
			break;
		}
		unlock(&authlock);
	}
	return s;
}

void
authfree(Auth *s)
{
	if(s != nil)
		s->inuse = 0;
}

int
authread(File* file, uchar* data, int n)
{
	Auth *s;
	int m;

	s = file->auth;
	if(s == nil)
		return -1;

	switch(s->phase){
	default:
		return failure(s, "unexpected phase");
	case HaveProtos:
		m = snprint((char*)data, n, "v.2 p9sk1@%s", nvr.authdom) + 1;
		s->phase = NeedProto;
		break;
	case HaveOK:
		m = 3;
		if(n < m)
			return failure(s, "read too short");
		strcpy((char*)data, "OK");
		s->phase = NeedCchal;
		break;
	case HaveSinfo:
		m = TICKREQLEN;
		if(n < m)
			return failure(s, "read too short");
		convTR2M(&s->tr, (char*)data);
		s->phase = NeedTicket;
		break;
	case HaveSauthenticator:
		m = AUTHENTLEN;
		if(n < m)
			return failure(s, "read too short");
		memmove(data, s->tbuf+TICKETLEN, m);
		s->phase = SSuccess;
		break;
	}
	return m;
}

int
authwrite(File* file, uchar *data, int n)
{
	Auth *s;
	int m;
	char *p, *d;
	Authenticator a;

	s = file->auth;
	if(s == nil)
		return -1;

	switch(s->phase){
	default:
		return failure(s, "unknown phase");
	case NeedProto:
		p = (char*)data;
		if(p[n-1] != 0)
			return failure(s, "proto missing terminator");
		d = strchr((char*)p, ' ');
		if(d == nil)
			return failure(s, "proto missing separator");
		*d++ = 0;
		if(strcmp(p, "p9sk1") != 0)
			return failure(s, "unknown proto");
		if(strcmp(d, nvr.authdom) != 0)
			return failure(s, "unknown domain");
		s->phase = HaveOK;
		m = n;
		break;
	case NeedCchal:
		m = CHALLEN;
		if(n < m)
			return failure(s, "client challenge too short");
		memmove(s->cchal, data, sizeof(s->cchal));
		s->phase = HaveSinfo;
		break;
	case NeedTicket:
		m = TICKETLEN+AUTHENTLEN;
		if(n < m)
			return failure(s, "ticket+auth too short");

		convM2T((char*)data, &s->t, nvr.authkey);
		if(s->t.num != AuthTs
		|| memcmp(s->t.chal, s->tr.chal, sizeof(s->t.chal)) != 0)
			return failure(s, "bad ticket");

		convM2A((char*)data+TICKETLEN, &a, s->t.key);
		if(a.num != AuthAc
		|| memcmp(a.chal, s->tr.chal, sizeof(a.chal)) != 0
		|| a.id != 0)
			return failure(s, "bad authenticator");

		/* at this point, we're convinced */
		s->uid = strtouid(s->t.suid);
		if(s->uid < 0)
			return failure(s, "unknown user");
		if(cons.flags & authdebugflag)
		print("user %s = %d authenticated\n", s->t.suid, s->uid);

		/* create an authenticator to send back */
		a.num = AuthAs;
		memmove(a.chal, s->cchal, sizeof(a.chal));
		a.id = 0;
		convA2M(&a, s->tbuf+TICKETLEN, s->t.key);

		s->phase = HaveSauthenticator;
		break;
	}
	return m;
}

int
authuid(Auth* s)
{
	return s->uid;
}

char*
authaname(Auth* s)
{
	return s->aname;
}

char*
authuname(Auth* s)
{
	return s->uname;
}
