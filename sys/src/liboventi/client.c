#include <u.h>
#include <libc.h>
#include <oventi.h>
#include "session.h"

static char EProtocolBotch[] = "venti protocol botch";
static char ELumpSize[] = "illegal lump size";
static char ENotConnected[] = "not connected to venti server";

static Packet *vtRPC(VtSession *z, int op, Packet *p);

VtSession *
vtClientAlloc(void)
{
	VtSession *z = vtAlloc();	
	return z;
}

VtSession *
vtDial(char *host, int canfail)
{
	VtSession *z;
	int fd;
	char *na;
	char e[ERRMAX];

	if(host == nil) 
		host = getenv("venti");
	if(host == nil)
		host = "$venti";

	if (host == nil) {
		if (!canfail)
			werrstr("no venti host set");
		na = "";
		fd = -1;
	} else {
		na = netmkaddr(host, 0, "venti");
		fd = dial(na, 0, 0, 0);
	}
	if(fd < 0){
		rerrstr(e, sizeof e);
		if(!canfail){
			vtSetError("venti dialstring %s: %s", na, e);
			return nil;
		}
	}
	z = vtClientAlloc();
	if(fd < 0)
		strcpy(z->fderror, e);
	vtSetFd(z, fd);
	return z;
}

int
vtRedial(VtSession *z, char *host)
{
	int fd;
	char *na;

	if(host == nil) 
		host = getenv("venti");
	if(host == nil)
		host = "$venti";

	na = netmkaddr(host, 0, "venti");
	fd = dial(na, 0, 0, 0);
	if(fd < 0){
		vtOSError();
		return 0;
	}
	vtReset(z);
	vtSetFd(z, fd);
	return 1;
}

VtSession *
vtStdioServer(char *server)
{
	int pfd[2];
	VtSession *z;

	if(server == nil)
		return nil;

	if(access(server, AEXEC) < 0) {
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
		execl(server, "ventiserver", "-i", nil);
		exits("exec failed");
	}
	close(pfd[1]);

	z = vtClientAlloc();
	vtSetFd(z, pfd[0]);
	return z;
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
vtSync(VtSession *z)
{
	Packet *p = packetAlloc();

	p = vtRPC(z, VtQSync, p);
	if(p == nil)
		return 0;
	if(packetSize(p) != 0){
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


static Packet *
vtRPC(VtSession *z, int op, Packet *p)
{
	uchar *hdr, buf[2];
	char *err;

	if(z == nil){
		vtSetError(ENotConnected);
		return nil;
	}

	/*
	 * single threaded for the momment
	 */
	vtLock(z->lk);
	if(z->cstate != VtStateConnected){
		vtSetError(ENotConnected);
		goto Err;
	}
	hdr = packetHeader(p, 2);
	hdr[0] = op;	/* op */
	hdr[1] = 0;	/* tid */
	vtDebug(z, "client send: ");
	vtDebugMesg(z, p, "\n");
	if(!vtSendPacket(z, p)) {
		p = nil;
		goto Err;
	}
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
