#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <venti.h>
#include "session.h"

/* score of a zero length block */
uchar	vtZeroScore[VtScoreSize] = {
	0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55,
	0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09
};

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
static char EProtocolBotch[] = "venti protocol botch";
static char ELumpSize[] = "illegal lump size";
static char EAuthState[] = "bad authentication state";
static char EVersion[] = "incorrect version number";
static char ENotServer[] = "not a server session";

static Packet *vtRPC(VtSession *z, int op, Packet *p);
static int vtSendPacket(VtSession *z, Packet *p);

static VtSession *
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

VtSession *
vtClientAlloc(void)
{
	VtSession *z = vtAlloc();	
	return z;
}

VtSession *
vtServerAlloc(VtServerVtbl *vtbl)
{
	VtSession *z = vtAlloc();
	z->vtbl = vtMemAlloc(sizeof(VtServerVtbl));
	*z->vtbl = *vtbl;
	return z;
}

VtSession *
vtDial(char *host)
{
	VtSession *z;
	int fd;
	char *na;

	if(host == nil) 
		host = getenv("venti");
	if(host == nil)
		host = "$venti";

	na = netmkaddr(host, 0, "venti");
	fd = dial(na, 0, 0, 0);
	if(fd < 0) {
		vtOSError();
		return nil;
	}
	z = vtClientAlloc();
	vtSetFd(z, fd);
	return z;
}

VtSession *
vtStdioServer(char *server)
{
	int pfd[2];
	VtSession *z;

	if(server == nil)
		return nil;

	if(access(server, OEXEC) < 0) {
		vtOSError();
		return nil;
	}

	if(pipe(pfd) < 0) {
		vtOSError();
		return nil;
	}

	switch(fork()) {
	case -1:
		close(pfd[0]);
		close(pfd[1]);
		vtOSError();
		return nil;
	case 0:
		close(pfd[0]);
		dup(pfd[1], 0);
		dup(pfd[1], 1);
		execl(server, "ventiserver", "-i", 0);
		exits("exec failed");
	}
	close(pfd[1]);

	z = vtClientAlloc();
	vtSetFd(z, pfd[0]);
	return z;
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

int
vtPing(VtSession *z)
{
	Packet *p = packetAlloc();

	p = vtRPC(z, VtQPing, p);
	if(p == nil)
		return 0;
	packetFree(p);
	return 1;
}

static int
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
	if(!packetConsume(p, (uchar*)s, n)) {
		vtMemFree(s);
		return 0;
	}
	s[n] = 0;
	*ret = s;
	return 1;
}

static int
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
		if(c < ' ' || c > 0x7f || *q && c != *q) {
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
	return 0;
}

int
vtHello(VtSession *z)
{
	Packet *p;
	uchar buf[10];
	char *sid;
	int crypto, codec;

	sid = nil;

	p = packetAlloc();
	if(!vtAddString(p, vtGetVersion(z)))
		goto Err;
	if(!vtAddString(p, vtGetUid(z)))
		goto Err;
	buf[0] = vtGetCryptoStrength(z);
	buf[1] = 0;
	buf[2] = 0;
	packetAppend(p, buf, 3);
	p = vtRPC(z, VtQHello, p);
	if(p == nil)
		return 0;
	if(!vtGetString(p, &sid))
		goto Err;
	if(!packetConsume(p, buf, 2))
		goto Err;
	if(packetSize(p) != 0) {
		vtSetError(EProtocolBotch);
		goto Err;
	}
	crypto = buf[0];
	codec = buf[1];

	USED(crypto);
	USED(codec);

	packetFree(p);

	vtLock(z->lk);
	z->sid = sid;
	z->auth.state = VtAuthOK;
	vtSha1Free(z->inHash);
	z->inHash = nil;
	vtSha1Free(z->outHash);
	z->outHash = nil;
	vtUnlock(z->lk);

	return 1;
Err:
	packetFree(p);
	vtMemFree(sid);
	return 0;
}

int
vtConnect(VtSession *z, char *password)
{
	char buf[VtMaxStringSize], *p, *prefix;
	int i;

	USED(password);
	vtLock(z->lk);
	if(z->cstate != VtStateAlloc) {
		vtSetError("bad session state");
		vtUnlock(z->lk);
		return 0;
	}

	/* be a little anal */
	vtLock(z->inLock);
	vtLock(z->outLock);

	prefix = "venti-";
	p = buf;
	p += sprintf(p, "%s", prefix);
	p += strlen(p);
	for(i=0; vtVersions[i].version; i++) {
		if(i != 0)
			*p++ = ':';
		p += sprintf(p, "%s", vtVersions[i].s);
	}
	p += sprintf(p, "-libventi\n");
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

int
vtWrite(VtSession *z, uchar score[VtScoreSize], int type, uchar *buf, int n)
{
	Packet *p = packetAlloc();

	packetAppend(p, buf, n);
	return vtWritePacket(z, score, type, p);
}

int
vtWritePacket(VtSession *z, uchar score[VtScoreSize], int type, Packet *p)
{
	int n = packetSize(p);
	uchar *hdr;

	if(n > VtMaxLumpSize || n < 0) {
		vtSetError(ELumpSize);
		goto Err;
	}
	
	if(n == 0) {
		memmove(score, vtZeroScore, VtScoreSize);
		return 1;
	}

	hdr = packetHeader(p, 4);
	hdr[0] = type;
	hdr[1] = 0;	/* pad */
	hdr[2] = 0;	/* pad */
	hdr[3] = 0;	/* pad */
	p = vtRPC(z, VtQWrite, p);
	if(p == nil)
		return 0;
	if(!packetConsume(p, score, VtScoreSize))
		goto Err;
	if(packetSize(p) != 0) {
		vtSetError(EProtocolBotch);
		goto Err;
	}
	packetFree(p);
	return 1;
Err:
	packetFree(p);
	return 0;
}

int
vtRead(VtSession *z, uchar score[VtScoreSize], int type, uchar *buf, int n)
{
	Packet *p;

	p = vtReadPacket(z, score, type, n);
	if(p == nil)
		return -1;
	n = packetSize(p);
	packetCopy(p, buf, 0, n);
	packetFree(p);
	return n;
}

Packet *
vtReadPacket(VtSession *z, uchar score[VtScoreSize], int type, int n)
{
	Packet *p;
	uchar buf[10];

	if(n < 0 || n > VtMaxLumpSize) {
		vtSetError(ELumpSize);
		return nil;
	}

	p = packetAlloc();
	if(memcmp(score, vtZeroScore, VtScoreSize) == 0)
		return p;

	packetAppend(p, score, VtScoreSize);
	buf[0] = type;
	buf[1] = 0;	/* pad */
	buf[2] = n >> 8;
	buf[3] = n;
	packetAppend(p, buf, 4);
	return vtRPC(z, VtQRead, p);
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
	if(z->eof) {
		vtUnlock(z->lk);
		return nil;
	}

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

static int
srvHello(VtSession *z, char *version, char *uid, int , uchar *, int , uchar *, int )
{
	vtLock(z->lk);
	if(z->auth.state != VtAuthHello) {
		vtSetError(EAuthState);
		goto Err;
	}
	if(strcmp(version, vtGetVersion(z)) != 0) {
		vtSetError(EVersion);
		goto Err;
	}
	vtMemFree(z->uid);
	z->uid = vtStrDup(uid);
	z->auth.state = VtAuthOK;
	vtUnlock(z->lk);
	return 1;
Err:
	z->auth.state = VtAuthFailed;
	vtUnlock(z->lk);
	return 0;
}


static int
dispatchHello(VtSession *z, Packet **pkt)
{
	char *version, *uid;
	uchar *crypto, *codec;
	uchar buf[10];
	int ncrypto, ncodec, cryptoStrength;
	int ret;
	Packet *p;

	p = *pkt;

	version = nil;	
	uid = nil;
	crypto = nil;
	codec = nil;

	ret = 0;
	if(!vtGetString(p, &version))
		goto Err;
	if(!vtGetString(p, &uid))
		goto Err;
	if(!packetConsume(p, buf, 2))
		goto Err;
	cryptoStrength = buf[0];
	ncrypto = buf[1];
	crypto = vtMemAlloc(ncrypto);
	if(!packetConsume(p, crypto, ncrypto))
		goto Err;

	if(!packetConsume(p, buf, 1))
		goto Err;
	ncodec = buf[0];
	codec = vtMemAlloc(ncodec);
	if(!packetConsume(p, codec, ncodec))
		goto Err;

	if(packetSize(p) != 0) {
		vtSetError(EProtocolBotch);
		goto Err;
	}
	if(!srvHello(z, version, uid, cryptoStrength, crypto, ncrypto, codec, ncodec)) {
		packetFree(p);
		*pkt = nil;
	} else {
		if(!vtAddString(p, vtGetSid(z)))
			goto Err;
		buf[0] = vtGetCrypto(z);
		buf[1] = vtGetCodec(z);
		packetAppend(p, buf, 2);
	}
	ret = 1;
Err:
	vtMemFree(version);
	vtMemFree(uid);
	vtMemFree(crypto);
	vtMemFree(codec);
	return ret;
}

static int
dispatchRead(VtSession *z, Packet **pkt)
{
	Packet *p;
	int type, n;
	uchar score[VtScoreSize], buf[4];

	p = *pkt;
	if(!packetConsume(p, score, VtScoreSize))
		return 0;
	if(!packetConsume(p, buf, 4))
		return 0;
	type = buf[0];
	n = (buf[2]<<8) | buf[3];
	if(packetSize(p) != 0) {
		vtSetError(EProtocolBotch);
		return 0;
	}
	packetFree(p);
	*pkt = (*z->vtbl->read)(z, score, type, n);
	return 1;
}

static int
dispatchWrite(VtSession *z, Packet **pkt)
{
	Packet *p;
	int type;
	uchar score[VtScoreSize], buf[4];

	p = *pkt;
	if(!packetConsume(p, buf, 4))
		return 0;
	type = buf[0];
	if(!(z->vtbl->write)(z, score, type, p)) {
		*pkt = 0;
	} else {
		*pkt = packetAlloc();
		packetAppend(*pkt, score, VtScoreSize);
	}
	return 1;
}

int
vtExport(VtSession *z)
{
	Packet *p;
	uchar buf[10], *hdr;
	int op, tid, clean;

	if(z->vtbl == nil) {
		vtSetError(ENotServer);
		return 0;
	}

	/* fork off slave */
	switch(rfork(RFNOWAIT|RFMEM|RFPROC)){
	case -1:
		vtOSError();
		return 0;
	case 0:
		break;
	default:
		return 1;
	}

	
	p = nil;
	clean = 0;
	vtAttach();
	if(!vtConnect(z, nil))
		goto Exit;

	vtDebug(z, "server connected!\n");
if(0)	vtSetDebug(z, 1);

	for(;;) {
		p = vtRecvPacket(z);
		if(p == nil) {
			break;
		}
		vtDebug(z, "server recv: ");
		vtDebugMesg(z, p, "\n");

		if(!packetConsume(p, buf, 2)) {
			vtSetError(EProtocolBotch);
			break;
		}
		op = buf[0];
		tid = buf[1];
		switch(op) {
		default:
			vtSetError(EProtocolBotch);
			goto Exit;
		case VtQPing:
			break;
		case VtQGoodbye:
			clean = 1;
			goto Exit;
		case VtQHello:
			if(!dispatchHello(z, &p))
				goto Exit;
			break;
		case VtQRead:
			if(!dispatchRead(z, &p))
				goto Exit;
			break;
		case VtQWrite:
			if(!dispatchWrite(z, &p))
				goto Exit;
			break;
		}
		if(p != nil) {
			hdr = packetHeader(p, 2);
			hdr[0] = op+1;
			hdr[1] = tid;
		} else {
			p = packetAlloc();
			hdr = packetHeader(p, 2);
			hdr[0] = VtRError;
			hdr[1] = tid;
			if(!vtAddString(p, vtGetError()))
				goto Exit;
		}

		vtDebug(z, "server send: ");
		vtDebugMesg(z, p, "\n");

		if(!vtSendPacket(z, p)) {
			p = nil;
			goto Exit;
		}
	}
Exit:
	if(p != nil)
		packetFree(p);
	if(z->vtbl->closing)
		z->vtbl->closing(z, clean);
	vtClose(z);
	vtFree(z);
	vtDetach();

	exits(0);
	return 0;	/* never gets here */
}

static int
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


static Packet *
vtRPC(VtSession *z, int op, Packet *p)
{
	uchar *hdr, buf[2];
	char *err;

	/*
	 * single threaded for the momment
	 */
	vtLock(z->lk);
	if(z->cstate != VtStateConnected)
		goto Err;
	hdr = packetHeader(p, 2);
	hdr[0] = op;	/* op */
	hdr[1] = 0;	/* tid */
	vtDebug(z, "client send: ");
	vtDebugMesg(z, p, "\n");
	if(!vtSendPacket(z, p))
		goto Err;
	p = vtRecvPacket(z);
	if(p == nil)
		goto Err;
	vtDebug(z, "client recv: ");
	vtDebugMesg(z, p, "\n");
	if(!packetConsume(p, buf, 2))
		goto Err;
	if(buf[0] == VtRError) {
		if(!vtGetString(p, &err)) {
			vtSetError(EProtocolBotch);
			goto Err;
		}
		vtSetError(err);
		vtMemFree(err);
		packetFree(p);
		vtUnlock(z->lk);
		return nil;
	}
	if(buf[0] != op+1 || buf[1] != 0) {
		vtSetError(EProtocolBotch);
		goto Err;
	}
	vtUnlock(z->lk);
	return p;
Err:
	vtDebug(z, "vtRPC failed: %s\n", vtGetError());
	if(p != nil)
		packetFree(p);
	vtUnlock(z->lk);
	vtDisconnect(z, 1);
	return nil;
}

void
vtFatal(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	fprintf(stderr, "fatal error: ");
	vfprintf(stderr, fmt, arg);
	fprintf(stderr, "\n");
	fflush(stderr);
	va_end(arg);
	exits("vtFatal");
}

void
vtDebug(VtSession *s, char *fmt, ...)
{
	va_list arg;

	if(!s->debug)
		return;

	va_start(arg, fmt);
	vfprintf(stderr, fmt, arg);
	fflush(stderr);
	va_end(arg);
}

void
vtDumpSome(Packet *pkt)
{
	int printable;
	int i, n;
	char buf[200], *q;
	uchar data[32], *p;

	n = packetSize(pkt);
	printable = 1;
	q = buf;
	q += sprintf(q, "(%d) '", n);

	if(n > sizeof(data))
		n = sizeof(data);
	p = packetPeek(pkt, data, 0, n);
	for(i=0; i<n && printable; i++)
		if((p[i]<32 && p[i] !='\n' && p[i] !='\t') || p[i]>127)
				printable = 0;
	if(printable) {
		for(i=0; i<n; i++)
			q += sprintf(q, "%c", p[i]);
	} else {
		for(i=0; i<n; i++) {
			if(i>0 && i%4==0)
				q += sprintf(q, " ");
			q += sprintf(q, "%.2X", p[i]);
		}
	}
	sprintf(q, "'");
	fprintf(stderr, "%s", buf);
}

void
vtDebugScore(uchar score[VtScoreSize])
{
	int i;

	for(i=0; i<VtScoreSize; i++)
		fprintf(stderr, "%.2x", score[i]);
}

void
vtDebugMesg(VtSession *z, Packet *p, char *s)
{
	int op;
	int tid;
	int n;
	uchar buf[100], *b;


	if(!z->debug)
		return;
	n = packetSize(p);
	if(n < 2) {
		fprintf(stderr, "runt packet%s", s);
		fflush(stderr);
		return;
	}
	b = packetPeek(p, buf, 0, 2);
	op = b[0];
	tid = b[1];

	fprintf(stderr, "%c%d[%d] %d", ((op&1)==0)?'R':'Q', op, tid, n);
	vtDumpSome(p);
	fprintf(stderr, "%s", s);
	fflush(stderr);
}

char*
vtStrDup(char *s)
{
	int n;
	char *ss;

	if(s == NULL)
		return NULL;
	n = strlen(s) + 1;
	ss = vtMemAlloc(n);
	memmove(ss, s, n);
	return ss;
}

int
vtFdReadFully(int fd, uchar *p, int n)
{
	int nn;

	while(n > 0) {
		nn = vtFdRead(fd, p, n);
		if(n <= 0)
			return 0;
		n -= nn;
		p += nn;
	}
	return 1;
}

int
vtZeroExtend(int type, uchar *buf, int n, int nn)
{
	uchar *p, *ep;

	switch(type) {
	default:
		memset(buf+n, 0, nn-n);
		break;
	case VtPointerType0:
	case VtPointerType1:
	case VtPointerType2:
	case VtPointerType3:
	case VtPointerType4:
	case VtPointerType5:
	case VtPointerType6:
	case VtPointerType7:
	case VtPointerType8:
	case VtPointerType9:
		p = buf + (n/VtScoreSize)*VtScoreSize;
		ep = buf + (nn/VtScoreSize)*VtScoreSize;
		while(p < ep) {
			memmove(p, vtZeroScore, VtScoreSize);
			p += VtScoreSize;
		}
		memset(p, 0, buf+nn-p);
		break;
	}
	return 1;
}

int 
vtZeroRetract(int type, uchar *buf, int n)
{
	uchar *p;

	switch(type) {
	default:
		for(p = buf + n; p > buf; p--) {
			if(p[-1] != 0)
				break;
		}
		return p - buf;
	case VtRootType:
		if(n < VtRootSize)
			return n;
		return VtRootSize;
	case VtPointerType0:
	case VtPointerType1:
	case VtPointerType2:
	case VtPointerType3:
	case VtPointerType4:
	case VtPointerType5:
	case VtPointerType6:
	case VtPointerType7:
	case VtPointerType8:
	case VtPointerType9:
		/* ignore slop at end of block */
		p = buf + (n/VtScoreSize)*VtScoreSize;

		while(p > buf) {
			if(memcmp(p - VtScoreSize, vtZeroScore, VtScoreSize) != 0)
				break;
			p -= VtScoreSize;
		}
		return p - buf;
	}
}
