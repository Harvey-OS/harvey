#include <u.h>
#include <libc.h>
#include <oventi.h>
#include "session.h"

struct {
	int version;
	char *s;
} vtVersions[] = {
	VtVersion02, "02",
	0, 0,
};

static char EBigString[] = "string too long";
static char EBigPacket[] = "packet too long";
static char ENullString[] = "missing string";
static char EBadVersion[] = "bad format in version string";

static Packet *vtRPC(VtSession *z, int op, Packet *p);


VtSession *
vtAlloc(void)
{
	VtSession *z;

	z = vtMemAllocZ(sizeof(VtSession));
	z->lk = vtLockAlloc();
//	z->inHash = vtSha1Alloc();
	z->inLock = vtLockAlloc();
	z->part = packetAlloc();
//	z->outHash = vtSha1Alloc();
	z->outLock = vtLockAlloc();
	z->fd = -1;
	z->uid = vtStrDup("anonymous");
	z->sid = vtStrDup("anonymous");
	return z;
}

void
vtReset(VtSession *z)
{
	vtLock(z->lk);
	z->cstate = VtStateAlloc;
	if(z->fd >= 0){
		vtFdClose(z->fd);
		z->fd = -1;
	}
	vtUnlock(z->lk);
}

int
vtConnected(VtSession *z)
{
	return z->cstate == VtStateConnected;
}

void
vtDisconnect(VtSession *z, int error)
{
	Packet *p;
	uchar *b;

vtDebug(z, "vtDisconnect\n");
	vtLock(z->lk);
	if(z->cstate == VtStateConnected && !error && z->vtbl == nil) {
		/* clean shutdown */
		p = packetAlloc();
		b = packetHeader(p, 2);
		b[0] = VtQGoodbye;
		b[1] = 0;
		vtSendPacket(z, p);
	}
	if(z->fd >= 0)
		vtFdClose(z->fd);
	z->fd = -1;
	z->cstate = VtStateClosed;
	vtUnlock(z->lk);
}

void
vtClose(VtSession *z)
{
	vtDisconnect(z, 0);
}

void
vtFree(VtSession *z)
{
	if(z == nil)
		return;
	vtLockFree(z->lk);
	vtSha1Free(z->inHash);
	vtLockFree(z->inLock);
	packetFree(z->part);
	vtSha1Free(z->outHash);
	vtLockFree(z->outLock);
	vtMemFree(z->uid);
	vtMemFree(z->sid);
	vtMemFree(z->vtbl);

	memset(z, 0, sizeof(VtSession));
	z->fd = -1;

	vtMemFree(z);
}

char *
vtGetUid(VtSession *s)
{
	return s->uid;
}

char *
vtGetSid(VtSession *z)
{
	return z->sid;
}

int
vtSetDebug(VtSession *z, int debug)
{
	int old;
	vtLock(z->lk);
	old = z->debug;
	z->debug = debug;
	vtUnlock(z->lk);
	return old;
}

int
vtSetFd(VtSession *z, int fd)
{
	vtLock(z->lk);
	if(z->cstate != VtStateAlloc) {
		vtSetError("bad state");
		vtUnlock(z->lk);
		return 0;
	}
	if(z->fd >= 0)
		vtFdClose(z->fd);
	z->fd = fd;
	vtUnlock(z->lk);
	return 1;
}

int
vtGetFd(VtSession *z)
{
	return z->fd;
}

int
vtSetCryptoStrength(VtSession *z, int c)
{
	if(z->cstate != VtStateAlloc) {
		vtSetError("bad state");
		return 0;
	}
	if(c != VtCryptoStrengthNone) {
		vtSetError("not supported yet");
		return 0;
	}
	return 1;
}

int
vtGetCryptoStrength(VtSession *s)
{
	return s->cryptoStrength;
}

int
vtSetCompression(VtSession *z, int fd)
{
	vtLock(z->lk);
	if(z->cstate != VtStateAlloc) {
		vtSetError("bad state");
		vtUnlock(z->lk);
		return 0;
	}
	z->fd = fd;
	vtUnlock(z->lk);
	return 1;
}

int
vtGetCompression(VtSession *s)
{
	return s->compression;
}

int
vtGetCrypto(VtSession *s)
{
	return s->crypto;
}

int
vtGetCodec(VtSession *s)
{
	return s->codec;
}

char *
vtGetVersion(VtSession *z)
{
	int v, i;
	
	v = z->version;
	if(v == 0)
		return "unknown";
	for(i=0; vtVersions[i].version; i++)
		if(vtVersions[i].version == v)
			return vtVersions[i].s;
	assert(0);
	return 0;
}

/* hold z->inLock */
static int
vtVersionRead(VtSession *z, char *prefix, int *ret)
{
	char c;
	char buf[VtMaxStringSize];
	char *q, *p, *pp;
	int i;

	q = prefix;
	p = buf;
	for(;;) {
		if(p >= buf + sizeof(buf)) {
			vtSetError(EBadVersion);
			return 0;
		}
		if(!vtFdReadFully(z->fd, (uchar*)&c, 1))
			return 0;
		if(z->inHash)
			vtSha1Update(z->inHash, (uchar*)&c, 1);
		if(c == '\n') {
			*p = 0;
			break;
		}
		if(c < ' ' || *q && c != *q) {
			vtSetError(EBadVersion);
			return 0;
		}
		*p++ = c;
		if(*q)
			q++;
	}
		
	vtDebug(z, "version string in: %s\n", buf);

	p = buf + strlen(prefix);
	for(;;) {
		for(pp=p; *pp && *pp != ':'  && *pp != '-'; pp++)
			;
		for(i=0; vtVersions[i].version; i++) {
			if(strlen(vtVersions[i].s) != pp-p)
				continue;
			if(memcmp(vtVersions[i].s, p, pp-p) == 0) {
				*ret = vtVersions[i].version;
				return 1;
			}
		}
		p = pp;
		if(*p != ':')
			return 0;
		p++;
	}	
}

Packet*
vtRecvPacket(VtSession *z)
{
	uchar buf[10], *b;
	int n;
	Packet *p;
	int size, len;

	if(z->cstate != VtStateConnected) {
		vtSetError("session not connected");
		return 0;
	}

	vtLock(z->inLock);
	p = z->part;
	/* get enough for head size */
	size = packetSize(p);
	while(size < 2) {
		b = packetTrailer(p, MaxFragSize);
		assert(b != nil);
		n = vtFdRead(z->fd, b, MaxFragSize);
		if(n <= 0)
			goto Err;
		size += n;
		packetTrim(p, 0, size);
	}

	if(!packetConsume(p, buf, 2))
		goto Err;
	len = (buf[0] << 8) | buf[1];
	size -= 2;

	while(size < len) {
		n = len - size;
		if(n > MaxFragSize)
			n = MaxFragSize;
		b = packetTrailer(p, n);
		if(!vtFdReadFully(z->fd, b, n))
			goto Err;
		size += n;
	}
	p = packetSplit(p, len);
	vtUnlock(z->inLock);
	return p;
Err:	
	vtUnlock(z->inLock);
	return nil;	
}

int
vtSendPacket(VtSession *z, Packet *p)
{
	IOchunk ioc;
	int n;
	uchar buf[2];
	
	/* add framing */
	n = packetSize(p);
	if(n >= (1<<16)) {
		vtSetError(EBigPacket);
		packetFree(p);
		return 0;
	}
	buf[0] = n>>8;
	buf[1] = n;
	packetPrefix(p, buf, 2);

	for(;;) {
		n = packetFragments(p, &ioc, 1, 0);
		if(n == 0)
			break;
		if(!vtFdWrite(z->fd, ioc.addr, ioc.len)) {
			packetFree(p);
			return 0;
		}
		packetConsume(p, nil, n);
	}
	packetFree(p);
	return 1;
}


int
vtGetString(Packet *p, char **ret)
{
	uchar buf[2];
	int n;
	char *s;

	if(!packetConsume(p, buf, 2))
		return 0;
	n = (buf[0]<<8) + buf[1];
	if(n > VtMaxStringSize) {
		vtSetError(EBigString);
		return 0;
	}
	s = vtMemAlloc(n+1);
	setmalloctag(s, getcallerpc(&p));
	if(!packetConsume(p, (uchar*)s, n)) {
		vtMemFree(s);
		return 0;
	}
	s[n] = 0;
	*ret = s;
	return 1;
}

int
vtAddString(Packet *p, char *s)
{
	uchar buf[2];
	int n;

	if(s == nil) {
		vtSetError(ENullString);
		return 0;
	}
	n = strlen(s);
	if(n > VtMaxStringSize) {
		vtSetError(EBigString);
		return 0;
	}
	buf[0] = n>>8;
	buf[1] = n;
	packetAppend(p, buf, 2);
	packetAppend(p, (uchar*)s, n);
	return 1;
}

int
vtConnect(VtSession *z, char *password)
{
	char buf[VtMaxStringSize], *p, *ep, *prefix;
	int i;

	USED(password);
	vtLock(z->lk);
	if(z->cstate != VtStateAlloc) {
		vtSetError("bad session state");
		vtUnlock(z->lk);
		return 0;
	}
	if(z->fd < 0){
		vtSetError("%s", z->fderror);
		vtUnlock(z->lk);
		return 0;
	}

	/* be a little anal */
	vtLock(z->inLock);
	vtLock(z->outLock);

	prefix = "venti-";
	p = buf;
	ep = buf + sizeof(buf);
	p = seprint(p, ep, "%s", prefix);
	p += strlen(p);
	for(i=0; vtVersions[i].version; i++) {
		if(i != 0)
			*p++ = ':';
		p = seprint(p, ep, "%s", vtVersions[i].s);
	}
	p = seprint(p, ep, "-libventi\n");
	assert(p-buf < sizeof(buf));
	if(z->outHash)
		vtSha1Update(z->outHash, (uchar*)buf, p-buf);
	if(!vtFdWrite(z->fd, (uchar*)buf, p-buf))
		goto Err;
	
	vtDebug(z, "version string out: %s", buf);

	if(!vtVersionRead(z, prefix, &z->version))
		goto Err;
		
	vtDebug(z, "version = %d: %s\n", z->version, vtGetVersion(z));

	vtUnlock(z->inLock);
	vtUnlock(z->outLock);
	z->cstate = VtStateConnected;
	vtUnlock(z->lk);

	if(z->vtbl)
		return 1;

	if(!vtHello(z))
		goto Err;
	return 1;	
Err:
	if(z->fd >= 0)
		vtFdClose(z->fd);
	z->fd = -1;
	vtUnlock(z->inLock);
	vtUnlock(z->outLock);
	z->cstate = VtStateClosed;
	vtUnlock(z->lk);
	return 0;	
}

