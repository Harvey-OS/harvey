#include <u.h>
#include <libc.h>
#include <ip.h>
#include <thread.h>
#include "netbios.h"

static int
decodehex(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return 0;
}

static char
encodehex(int n)
{
	if (n >= 0 && n <= 9)
		return '0' + n;
	if (n >= 10 && n <= 15)
		return 'a' + (n - 10);
	return '?';
}

static int
_nameextract(uchar *base, uchar *p, uchar *ep, int k, uchar *outbuf, int outbufmaxlen, int *outbuflenp)
{
	uchar *op, *oep, *savep;
	savep = p;
	op = outbuf;
	oep = outbuf + outbufmaxlen;
	for (;;) {
		uchar b;
		int n;
		if (p >= ep)
			return 0;
		b = *p++;
		if (b == 0)
			break;
		if (k) {
			if (op >= oep)
				return 0;
			*op++ = '.';
		}
		if ((b & 0xc0) == 0xc0) {
			ushort off;
			if (ep - p < 2)
				return 0;
			off = nhgets(p - 1) & 0x3fff; p++;
			if (_nameextract(base, base + off, p, k, op, oep - ep, &n) == 0)
				return 0;
			op += n;
		}
		else if ((b & 0xc0) != 0x00)
			return 0;
		else if (b != 0x20)
			return 0;
		else {
			int x;
			if (p + b > ep)
				return 0;
			if (op + b / 2 > oep)
				return 0;
			for (x = 0; x < b; x += 2) {
				uchar hn, ln;
				if (*p < 'A' || *p >= 'A' + 16)
					return 0;
				hn = *p++ - 'A';
				if (*p < 'A' || *p >= 'A' + 16)
					return 0;
				ln = *p++ - 'A';
				*op++ = (hn << 4) | ln;
			}
		}
		k++;
	}
	*outbuflenp = op - outbuf;
	return p - savep;
}

int
nbnamedecode(uchar *base, uchar *p, uchar *ep, NbName nbname)
{
	int n;
	int rv = _nameextract(base, p, ep, 0, nbname, NbNameLen, &n);
	if (rv == 0)
		return rv;
	if (n != NbNameLen)
		return 0;
	return rv;
}

int
nbnameencode(uchar *ap, uchar *ep, NbName name)
{
	uchar *p = ap;
	int i;
	if (p + 1 + 2 * NbNameLen + 1 > ep)
		return 0;
	*p++ = NbNameLen * 2;
	for (i = 0; i < NbNameLen; i++) {
		*p++ = 'A' + ((name[i] >> 4) & 0xf);
		*p++ = 'A' + (name[i] & 0xf);
	}
	*p++ = 0;
	return p - ap;
}

void
nbnamecpy(NbName n1, NbName n2)
{
	memcpy(n1, n2, NbNameLen);
}

void
nbmknamefromstring(NbName nbname, char *s)
{
	int i;
	memset(nbname, ' ', NbNameLen - 1);
	nbname[NbNameLen - 1] = 0;
	i = 0;
	while (*s) {
		if (*s == '\\' && *(s + 1) == 'x') {
			s += 2;
			if (*s == 0 || *(s + 1) == 0)
				return;
			nbname[NbNameLen - 1] = (decodehex(s[0]) << 4) | decodehex(s[1]);
			return;
		} else {
			if (i < NbNameLen - 1)
				nbname[i++] = toupper(*s);
			s++;
		}
	}
}

void
nbmknamefromstringandtype(NbName nbname, char *s, uchar type)
{
	nbmknamefromstring(nbname, s);
	nbname[NbNameLen - 1] = type;
}

void
nbmkstringfromname(char *buf, int buflen, NbName name)
{
	int x;
	for (x = 0; x < NbNameLen - 1; x++) {
		if (name[x] == ' ')
			break;
		if (buflen > 1) {
			*buf++ = tolower(name[x]);
			buflen--;
		}
	}
	if (name[NbNameLen - 1] != 0) {
		if (buflen > 1) {
			*buf++ = '\\';
			buflen--;
		}
		if (buflen > 1) {
			*buf++ = 'x';
			buflen--;
		}
		if (buflen > 1) {
			*buf++ = encodehex(name[NbNameLen - 1] >> 4);
			buflen--;
		}
		if (buflen > 1)
			*buf++ = encodehex(name[NbNameLen - 1] & 0x0f);
	}
	*buf = 0;
}

int
nbnameisany(NbName name)
{
	return name[0] == '*';
}

int
nbnameequal(NbName name1, NbName name2)
{
	if (name1[NbNameLen - 1] != name2[NbNameLen - 1])
		return 0;
	if (nbnameisany(name1))
		return 1;
	if (nbnameisany(name2))
		return 1;
	return memcmp(name1, name2, NbNameLen - 1) == 0;
}

int
nbnamefmt(Fmt *f)
{
	uchar *n;
	char buf[100];
	n = va_arg(f->args, uchar *);
	nbmkstringfromname(buf, sizeof(buf), n);
	return fmtstrcpy(f, buf);
}

typedef struct NbNameTableEntry NbNameTableEntry;
struct NbNameTableEntry {
	NbName name;
	NbNameTableEntry *next;
};

static struct {
	QLock;
	NbNameTableEntry *head;
} nbnametable;

int
nbnametablefind(NbName name, int add)
{
	int rv;
	NbNameTableEntry *p;
	qlock(&nbnametable);
	for (p = nbnametable.head; p; p = p->next) {
		if (nbnameequal(p->name, name)) {
			qunlock(&nbnametable);
			return 1;
		}
	}
	if (add) {
		p = nbemalloc(sizeof(*p));
		nbnamecpy(p->name, name);
		p->next = nbnametable.head;
		nbnametable.head = p;
		rv = 1;
	}
	else
		rv = 0;
	qunlock(&nbnametable);
	return rv;
}

typedef struct NbRemoteNameTableEntry NbRemoteNameTableEntry;
struct NbRemoteNameTableEntry {
	NbName name;
	char ipaddr[IPaddrlen];
	long expire;
	NbRemoteNameTableEntry *next;
};

static struct {
	QLock;
	NbRemoteNameTableEntry *head;
} nbremotenametable;

int
nbremotenametablefind(NbName name, uchar *ipaddr)
{
	NbRemoteNameTableEntry *p, **pp;
	long now = time(nil);
	qlock(&nbremotenametable);
	for (pp = &nbremotenametable.head; (p = *pp) != nil;) {
		if (p->expire <= now) {
//print("nbremotenametablefind: expired %B\n", p->name);
			*pp = p->next;
			free(p);
			continue;
		}
		if (nbnameequal(p->name, name)) {
			ipmove(ipaddr, p->ipaddr);
			qunlock(&nbremotenametable);
			return 1;
		}
		pp = &p->next;
	}
	qunlock(&nbremotenametable);
	return 0;
}

int
nbremotenametableadd(NbName name, uchar *ipaddr, ulong ttl)
{
	NbRemoteNameTableEntry *p;
	qlock(&nbremotenametable);
	for (p = nbremotenametable.head; p; p = p->next)
		if (nbnameequal(p->name, name))
			break;
	if (p == nil) {
		p = nbemalloc(sizeof(*p));
		p->next = nbremotenametable.head;
		nbremotenametable.head = p;
		nbnamecpy(p->name, name);
	}
	ipmove(p->ipaddr, ipaddr);
	p->expire = time(nil) + ttl;
//print("nbremotenametableadd: %B ttl %lud expire %ld\n", p->name, ttl, p->expire);
	qunlock(&nbremotenametable);
	return 1;
}
