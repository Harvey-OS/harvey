#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <String.h>

#include "/sys/src/libauth/authlocal.h"
#include "dat.h"

char *Eproto = "bad key or protocol botch";
char *Ephase = "protocol phase error";
char *Etoosmall = "read/write too short";

typedef struct State State;
struct State 
{
	int	phase;
	int	version;
	char	*user;
	char	cchal[CHALLEN];
	char	mykey[DESKEYLEN];
	char	tbuf[TICKETLEN+AUTHENTLEN];	/* server ticket */
	Ticket	t;
	Ticketreq tr;
};

enum
{
	/* client phases */
	HaveCchal,
	NeedSinfo,
	HaveTicket,
	NeedSauthenticator,

	/* server phases */
	NeedCchal,
	HaveSinfo,
	NeedTicket,
	HaveSauthenticator,

	/* common to both */
	Success,
	HaveInfo,
};

/*
 *  sk1 is the more complete, the client sends first
 */
static void*
p9sk1_open(Key *k, char *user, int client)
{
	State *s;

	s = emalloc(sizeof(*s));
	if(client){
		s->phase = HaveCchal;
		memrandom(s->cchal, sizeof s->cchal);
	} else {
		s->phase = NeedCchal;
		s->tr.type = AuthTreq;
		safecpy(s->tr.authid, user, ANAMELEN);
		safecpy(s->tr.authdom, "who cares", DOMLEN);
		memrandom(s->tr.chal, sizeof s->tr.chal);
	}
	s->version = 1;
	s->user = user;
	passtokey(s->mykey, s_to_c(k->data));

	return s;
}

/*
 *  sk2 doesn't use the client challenge so server sends first
 */
static void*
p9sk2_open(Key *k, char *user, int client)
{
	State *s;

	s = emalloc(sizeof(*s));
	if(client){
		s->phase = NeedSinfo;
	} else {
		s->phase = HaveSinfo;
		s->tr.type = AuthTreq;
		safecpy(s->tr.authid, user, ANAMELEN);
		safecpy(s->tr.authdom, "who cares", DOMLEN);
		memrandom(s->tr.chal, sizeof s->tr.chal);
		memmove(s->cchal, s->tr.chal, sizeof s->cchal);
	}
	s->version = 2;
	s->user = user;
	passtokey(s->mykey, s_to_c(k->data));

	return s;
}

static int
p9sk1_read(void *sp, void *vdata, int n)
{
	int m;
	State *s = sp;
	char *data = vdata;

	switch(s->phase){
	default:
		werrstr(Ephase);
		return -1;
	case HaveCchal:				/* have client challenge */
		m = CHALLEN;
		if(n < m){
			werrstr(Etoosmall);
			return -1;
		}
		memmove(data, s->cchal, sizeof(s->cchal));
		s->phase = NeedSinfo;
		break;
	case HaveSinfo:
		m = TICKREQLEN;
		if(n < m){
			werrstr(Etoosmall);
			return -1;
		}
		convTR2M(&s->tr, data);
		s->phase = NeedTicket;
		break;
	case HaveTicket:
		m = TICKREQLEN+AUTHENTLEN;
		if(n < m){
			werrstr(Etoosmall);
			return -1;
		}
		memmove(data, s->tbuf, m);
		s->phase = NeedSauthenticator;
		break;
	case HaveSauthenticator:
		m = AUTHENTLEN;
		if(n < m){
			werrstr(Etoosmall);
			return -1;
		}
		memmove(data, s->tbuf+TICKETLEN, m);
		s->phase = Success;
		break;
	case Success:
		m = 0;
		s->phase = HaveInfo;
		break;
	case HaveInfo:
		m = 0;
		break;
	}
	return m;
}

static int
p9sk1_write(void *sp, void *vdata, int n)
{
	int rv, afd;
	char trbuf[TICKREQLEN];
	char tbuf[TICKETLEN];
	int m;
	State *s = sp;
	char *data = vdata;
	Authenticator a;

	switch(s->phase){
	default:
		werrstr(Ephase);
		return -1;
	case NeedCchal:			/* server needs client challenge */
		m = CHALLEN;
		if(n < m){
			werrstr(Etoosmall);
			return -1;
		}
		memmove(s->cchal, data, sizeof(s->cchal));
		s->phase = HaveSinfo;
		break;
	case NeedSinfo:			/* client needs ticket request */
		/* get a ticket request */
		m = TICKREQLEN;
		if(n < m){
			werrstr(Etoosmall);
			return -1;
		}

		/* remember server's chal */
		convM2TR(data, &s->tr);
		if(s->version == 2)
			memmove(s->cchal, s->tr.chal, sizeof(s->cchal));


		/* fill in the rest of the request */
		s->tr.type = AuthTreq;
		safecpy(s->tr.hostid, user, sizeof(s->tr.hostid));
		safecpy(s->tr.uid, s->user, sizeof(s->tr.uid));
		convTR2M(&s->tr, trbuf);

		/* get my and server's ticket from auth server */
		afd = authdial();
		if(afd < 0)
			return -1;
		rv = _asgetticket(afd, trbuf, tbuf);
		close(afd);
		if(rv < 0)
			return -1;
		convM2T(tbuf, &s->t, s->mykey);
		if(s->t.num != AuthTc){
			werrstr("invalid key");
			return -1;
		}

		/* save server ticket in state */
		memmove(s->tbuf, tbuf+TICKETLEN, TICKETLEN);

		/* add authenticator to ticket */
		a.num = AuthAc;
		memmove(a.chal, s->tr.chal, sizeof(a.chal));
		a.id = 0;
		convA2M(&a, s->tbuf+TICKETLEN, s->t.key);

		s->phase = HaveTicket;
		break;
	case NeedTicket:
		m = TICKREQLEN+AUTHENTLEN;
		if(n < m){
			werrstr(Etoosmall);
			return -1;
		}
		convM2T(data, &s->t, s->mykey);
		if(s->t.num != AuthTs
		|| memcmp(s->t.chal, s->tr.chal, sizeof(s->t.chal)) != 0){
			werrstr(Eproto);
			return -1;
		}
		convM2A(data+TICKETLEN, &a, s->t.key);
		if(a.num != AuthAc
		|| memcmp(a.chal, s->tr.chal, sizeof(a.chal)) != 0
		|| a.id != 0){
			werrstr(Eproto);
			return -1;
		}

		/* create an authenticator to send back */
		a.num = AuthAs;
		memmove(a.chal, s->cchal, sizeof(a.chal));
		a.id = 0;
		convA2M(&a, s->tbuf+TICKETLEN, s->t.key);

		s->phase = HaveSauthenticator;
		break;
	case NeedSauthenticator:
		m = AUTHENTLEN;
		if(n < m){
			werrstr(Etoosmall);
			return -1;
		}
		convM2A(data, &a, s->t.key);
		if(a.num != AuthAs
		|| memcmp(a.chal, s->cchal, sizeof(s->t.chal)) != 0
		|| a.id != 0){
			werrstr(Eproto);
			return -1;
		}

		s->phase = Success;
		break;
	}
	return m;
}


static void
p9sk1_close(void *s)
{
	if(s != nil)
		free(s);
}

Proto p9sk1 =
{
.name=	"p9sk1",
.open=	p9sk1_open,
.write=	p9sk1_write,
.read=	p9sk1_read,
.close=	p9sk1_close,
};

Proto p9sk2 =
{
.name=	"p9sk2",
.open=	p9sk2_open,
.write=	p9sk1_write,
.read=	p9sk1_read,
.close=	p9sk1_close,
};
