/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * This code uses RADIUS as a portable way to validate tokens such as SecurID.
 * It is relatively simple to send a UDP packet and get a response, but various
 * things can go wrong.  Speaking the proprietary ACE protocol would allow
 * handling "next token code" and other error messages.  More importantly, the
 * timeout threshold is inherently hard to pick.  We observe responses taking
 * longer than 10 seconds in normal times.  That is a long time to wait before
 * retrying on a second server.  Moreover, if the UDP response is lost, retrying
 * on a second server will also fail because the valid token code may be
 * presented only once.  This whole approach is flawed, but best we can do.
 */
/* RFC2138 */
#include <u.h>
#include <libc.h>
#include <ip.h>
#include <ctype.h>
#include <mp.h>
#include <libsec.h>
#include <bio.h>
#include <ndb.h>

#define AUTHLOG "auth"

enum { R_AccessRequest = 1, /* Packet code */
       R_AccessAccept = 2,
       R_AccessReject = 3,
       R_AccessChallenge = 11,
       R_UserName = 1,
       R_UserPassword = 2,
       R_NASIPAddress = 4,
       R_ReplyMessage = 18,
       R_State = 24,
       R_NASIdentifier = 32,
};

typedef struct Secret {
	uint8_t* s;
	int len;
} Secret;

typedef struct Attribute {
	struct Attribute* next;
	uint8_t type;
	uint8_t len; /* number of bytes in value */
	uint8_t val[256];
} Attribute;

typedef struct Packet {
	uint8_t code, ID;
	uint8_t authenticator[16];
	Attribute first;
} Packet;

/* assumes pass is at most 16 chars */
void
hide(Secret* shared, uint8_t* auth, Secret* pass, uint8_t* x)
{
	DigestState* M;
	int i, n = pass->len;

	M = md5(shared->s, shared->len, nil, nil);
	md5(auth, 16, x, M);
	if(n > 16)
		n = 16;
	for(i = 0; i < n; i++)
		x[i] ^= pass->s[i];
}

int
authcmp(Secret* shared, uint8_t* buf, int m, uint8_t* auth)
{
	DigestState* M;
	uint8_t x[16];

	M = md5(buf, 4, nil, nil);         /* Code+ID+Length */
	M = md5(auth, 16, nil, M);         /* RequestAuth */
	M = md5(buf + 20, m - 20, nil, M); /* Attributes */
	md5(shared->s, shared->len, x, M);
	return memcmp(x, buf + 4, 16);
}

Packet*
newRequest(uint8_t* auth)
{
	static uint8_t ID = 0;
	Packet* p;

	p = (Packet*)malloc(sizeof(*p));
	if(p == nil)
		return nil;
	p->code = R_AccessRequest;
	p->ID = ++ID;
	memmove(p->authenticator, auth, 16);
	p->first.next = nil;
	p->first.type = 0;
	return p;
}

void
freePacket(Packet* p)
{
	Attribute* a, *x;

	if(!p)
		return;
	a = p->first.next;
	while(a) {
		x = a;
		a = a->next;
		free(x);
	}
	free(p);
}

int
ding(void* v, char* msg)
{
	syslog(0, AUTHLOG, "ding %s", msg);
	if(strstr(msg, "alarm"))
		return 1;
	return 0;
}

Packet*
rpc(char* dest, Secret* shared, Packet* req)
{
	uint8_t buf[4096], buf2[4096], *b, *e;
	Packet* resp;
	Attribute* a;
	int m, n, fd, try
		;

	/* marshal request */
	e = buf + sizeof buf;
	buf[0] = req->code;
	buf[1] = req->ID;
	memmove(buf + 4, req->authenticator, 16);
	b = buf + 20;
	for(a = &req->first; a; a = a->next) {
		if(b + 2 + a->len > e)
			return nil;
		*b++ = a->type;
		*b++ = 2 + a->len;
		memmove(b, a->val, a->len);
		b += a->len;
	}
	n = b - buf;
	buf[2] = n >> 8;
	buf[3] = n;

	/* send request, wait for reply */
	fd = dial(dest, 0, 0, 0);
	if(fd < 0) {
		syslog(0, AUTHLOG, "%s: rpc can't get udp channel", dest);
		return nil;
	}
	atnotify(ding, 1);
	m = -1;
	for(try = 0; try < 2; try ++) {
		/*
		 * increased timeout from 4sec to 15sec because
		 * corporate server really takes that long.
		 */
		alarm(15000);
		m = write(fd, buf, n);
		if(m != n) {
			syslog(0, AUTHLOG, "%s: rpc write err %d %d: %r", dest,
			       m, n);
			m = -1;
			break;
		}
		m = read(fd, buf2, sizeof buf2);
		alarm(0);
		if(m < 0) {
			syslog(0, AUTHLOG, "%s rpc read err %d: %r", dest, m);
			break; /* failure */
		}
		if(m == 0 || buf2[1] != buf[1]) { /* need matching ID */
			syslog(0, AUTHLOG, "%s unmatched reply %d", dest, m);
			continue;
		}
		if(authcmp(shared, buf2, m, buf + 4) == 0)
			break;
		syslog(0, AUTHLOG, "%s bad rpc chksum", dest);
	}
	close(fd);
	if(m <= 0)
		return nil;

	/* unmarshal reply */
	b = buf2;
	e = buf2 + m;
	resp = (Packet*)malloc(sizeof(*resp));
	if(resp == nil)
		return nil;
	resp->code = *b++;
	resp->ID = *b++;
	n = *b++;
	n = (n << 8) | *b++;
	if(m != n) {
		syslog(0, AUTHLOG, "rpc got %d bytes, length said %d", m, n);
		if(m > n)
			e = buf2 + n;
	}
	memmove(resp->authenticator, b, 16);
	b += 16;
	a = &resp->first;
	a->type = 0;
	for(;;) {
		if(b >= e) {
			a->next = nil;
			break; /* exit loop */
		}
		a->type = *b++;
		a->len = (*b++) - 2;
		if(b + a->len > e) { /* corrupt packet */
			a->next = nil;
			freePacket(resp);
			return nil;
		}
		memmove(a->val, b, a->len);
		b += a->len;
		if(b < e) { /* any more attributes? */
			a->next = (Attribute*)malloc(sizeof(*a));
			if(a->next == nil) {
				free(req);
				return nil;
			}
			a = a->next;
		}
	}
	return resp;
}

int
setAttribute(Packet* p, uint8_t type, uint8_t* s, int n)
{
	Attribute* a;

	a = &p->first;
	if(a->type != 0) {
		a = (Attribute*)malloc(sizeof(*a));
		if(a == nil)
			return -1;
		a->next = p->first.next;
		p->first.next = a;
	}
	a->type = type;
	a->len = n;
	if(a->len > 253) /* RFC2138, section 5 */
		a->len = 253;
	memmove(a->val, s, a->len);
	return 0;
}

/* return a reply message attribute string */
char*
replymsg(Packet* p)
{
	Attribute* a;
	static char buf[255];

	for(a = &p->first; a; a = a->next)
		if(a->type == R_ReplyMessage) {
			if(a->len >= sizeof buf)
				a->len = sizeof(buf) - 1;
			memmove(buf, a->val, a->len);
			buf[a->len] = 0;
		}
	return buf;
}

/* for convenience while debugging */
char* replymess;
Attribute* stateattr;

void
logPacket(Packet* p)
{
	int i;
	char* np, *e;
	char buf[255], pbuf[4 * 1024];
	uint8_t* au = p->authenticator;
	Attribute* a;

	e = pbuf + sizeof(pbuf);

	np = seprint(pbuf, e, "Packet ID=%d auth=%x %x %x... ", p->ID, au[0],
	             au[1], au[2]);
	switch(p->code) {
	case R_AccessRequest:
		np = seprint(np, e, "request\n");
		break;
	case R_AccessAccept:
		np = seprint(np, e, "accept\n");
		break;
	case R_AccessReject:
		np = seprint(np, e, "reject\n");
		break;
	case R_AccessChallenge:
		np = seprint(np, e, "challenge\n");
		break;
	default:
		np = seprint(np, e, "code=%d\n", p->code);
		break;
	}
	replymess = "0000000";
	for(a = &p->first; a; a = a->next) {
		if(a->len > 253)
			a->len = 253;
		memmove(buf, a->val, a->len);
		np = seprint(np, e, " [%d]", a->type);
		for(i = 0; i < a->len; i++)
			if(isprint(a->val[i]))
				np = seprint(np, e, "%c", a->val[i]);
			else
				np = seprint(np, e, "\\%o", a->val[i]);
		np = seprint(np, e, "\n");
		buf[a->len] = 0;
		if(a->type == R_ReplyMessage)
			replymess = strdup(buf);
		else if(a->type == R_State)
			stateattr = a;
	}

	syslog(0, AUTHLOG, "%s", pbuf);
}

static uint8_t*
getipv4addr(void)
{
	Ipifc* nifc;
	Iplifc* lifc;
	static Ipifc* ifc;

	ifc = readipifc("/net", ifc, -1);
	for(nifc = ifc; nifc; nifc = nifc->next)
		for(lifc = nifc->lifc; lifc; lifc = lifc->next)
			if(ipcmp(lifc->ip, IPnoaddr) != 0 &&
			   ipcmp(lifc->ip, v4prefix) != 0)
				return lifc->ip;
	return nil;
}

extern Ndb* db;

/* returns 0 on success, error message on failure */
char*
secureidcheck(char* user, char* response)
{
	char* radiussecret = nil;
	char* rv = "authentication failed";
	char dest[3 * IPaddrlen + 20], ruser[64];
	uint8_t* ip;
	uint8_t x[16];
	uint32_t u[4];
	Ndbs s;
	Ndbtuple* t = nil, *nt, *tt;
	Packet* req = nil, * resp = nil;
	Secret shared, pass;
	static Ndb* netdb;

	if(netdb == nil)
		netdb = ndbopen(0);

	/* bad responses make them disable the fob, avoid silly checks */
	if(strlen(response) < 4 || strpbrk(response, "abcdefABCDEF") != nil)
		goto out;

	/* get radius secret */
	radiussecret =
	    ndbgetvalue(db, &s, "radius", "lra-radius", "secret", &t);
	if(radiussecret == nil) {
		syslog(0, AUTHLOG, "secureidcheck: nil radius secret: %r");
		goto out;
	}

	/* translate user name if we have to */
	strcpy(ruser, user);
	for(nt = t; nt; nt = nt->entry)
		if(strcmp(nt->attr, "uid") == 0 && strcmp(nt->val, user) == 0)
			for(tt = nt->line; tt != nt; tt = tt->line)
				if(strcmp(tt->attr, "rid") == 0) {
					strcpy(ruser, tt->val);
					break;
				}
	ndbfree(t);
	t = nil;

	u[0] = fastrand();
	u[1] = fastrand();
	u[2] = fastrand();
	u[3] = fastrand();
	req = newRequest((uint8_t*)u);
	if(req == nil)
		goto out;
	shared.s = (uint8_t*)radiussecret;
	shared.len = strlen(radiussecret);
	ip = getipv4addr();
	if(ip == nil) {
		syslog(0, AUTHLOG, "no interfaces: %r\n");
		goto out;
	}
	if(setAttribute(req, R_NASIPAddress, ip + IPv4off, 4) < 0)
		goto out;

	if(setAttribute(req, R_UserName, (uint8_t*)ruser, strlen(ruser)) < 0)
		goto out;
	pass.s = (uint8_t*)response;
	pass.len = strlen(response);
	hide(&shared, req->authenticator, &pass, x);
	if(setAttribute(req, R_UserPassword, x, 16) < 0)
		goto out;

	t = ndbsearch(netdb, &s, "sys", "lra-radius");
	if(t == nil) {
		syslog(0, AUTHLOG,
		       "secureidcheck: nil radius sys search: %r\n");
		goto out;
	}
	for(nt = t; nt; nt = nt->entry) {
		if(strcmp(nt->attr, "ip") != 0)
			continue;

		snprint(dest, sizeof dest, "udp!%s!radius", nt->val);
		resp = rpc(dest, &shared, req);
		if(resp == nil) {
			syslog(0, AUTHLOG, "%s nil response", dest);
			continue;
		}
		if(resp->ID != req->ID) {
			syslog(0, AUTHLOG, "%s mismatched ID  req=%d resp=%d",
			       dest, req->ID, resp->ID);
			freePacket(resp);
			resp = nil;
			continue;
		}

		switch(resp->code) {
		case R_AccessAccept:
			syslog(0, AUTHLOG, "%s accepted ruser=%s", dest, ruser);
			rv = nil;
			break;
		case R_AccessReject:
			syslog(0, AUTHLOG, "%s rejected ruser=%s %s", dest,
			       ruser, replymsg(resp));
			rv = "secureid failed";
			break;
		case R_AccessChallenge:
			syslog(0, AUTHLOG, "%s challenge ruser=%s %s", dest,
			       ruser, replymsg(resp));
			rv = "secureid out of sync";
			break;
		default:
			syslog(0, AUTHLOG, "%s code=%d ruser=%s %s", dest,
			       resp->code, ruser, replymsg(resp));
			break;
		}
		break; /* we have a proper reply, no need to ask again */
	}
out:
	if(t)
		ndbfree(t);
	free(radiussecret);
	freePacket(req);
	freePacket(resp);
	return rv;
}
