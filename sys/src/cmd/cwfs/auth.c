#include "all.h"
#include "io.h"
#include <authsrv.h>

Nvrsafe	nvr;

static int gotnvr;	/* flag: nvr contains nvram; it could be bad */

char*
nvrgetconfig(void)
{
	return conf.confdev;
}

/*
 * we shouldn't be writing nvram any more.
 * the secstore/config field is now just secstore key.
 * we still use authid, authdom and machkey for authentication.
 */

int
nvrcheck(void)
{
	uchar csum;

	if (readnvram(&nvr, NVread) < 0) {
		print("nvrcheck: can't read nvram\n");
		return 1;
	} else
		gotnvr = 1;
	print("nvr read\n");

	csum = nvcsum(nvr.machkey, sizeof nvr.machkey);
	if(csum != nvr.machsum) {
		print("\n\n ** NVR key checksum is incorrect  **\n");
		print(" ** set password to allow attaches **\n\n");
		memset(nvr.machkey, 0, sizeof nvr.machkey);
		return 1;
	}

	return 0;
}

int
nvrsetconfig(char* word)
{
	/* config block is on device `word' */
	USED(word);
	return 0;
}

int
conslock(void)
{
	char *ln;
	char nkey1[DESKEYLEN];
	static char zeroes[DESKEYLEN];

	if(memcmp(nvr.machkey, zeroes, DESKEYLEN) == 0) {
		print("no password set\n");
		return 0;
	}

	for(;;) {
		print("%s password:", service);
		/* could turn off echo here */

		if ((ln = Brdline(&bin, '\n')) == nil)
			return 0;
		ln[Blinelen(&bin)-1] = '\0';

		/* could turn on echo here */
		memset(nkey1, 0, DESKEYLEN);
		passtokey(nkey1, ln);
		if(memcmp(nkey1, nvr.machkey, DESKEYLEN) == 0) {
			prdate();
			break;
		}

		print("Bad password\n");
		delay(1000);
	}
	return 1;
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
	Userid	uid;		/* uid decided on */
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
	auths = malloc(conf.nauth * sizeof(*auths));
}

static int
failure(Auth *s, char *why)
{
	int i;

if(*why)print("authentication failed: %s: %s\n", phasename[s->phase], why);
	srand((uintptr)s + time(nil));
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
	int i, nwrap;
	Auth *s;

	i = si;
	nwrap = 0;
	for(;;){
		if(i < 0 || i >= conf.nauth){
			if(++nwrap > 1)
				return nil;
			i = 0;
		}
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
		d = strchr(p, ' ');
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

		convM2T((char*)data, &s->t, nvr.machkey);
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
			print("user %s = %d authenticated\n",
				s->t.suid, s->uid);

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
