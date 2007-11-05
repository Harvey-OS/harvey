#include <u.h>
#include <libc.h>
#include <oventi.h>
#include "session.h"

static char EAuthState[] = "bad authentication state";
static char ENotServer[] = "not a server session";
static char EVersion[] = "incorrect version number";
static char EProtocolBotch[] = "venti protocol botch";

VtSession *
vtServerAlloc(VtServerVtbl *vtbl)
{
	VtSession *z = vtAlloc();
	z->vtbl = vtMemAlloc(sizeof(VtServerVtbl));
	setmalloctag(z->vtbl, getcallerpc(&vtbl));
	*z->vtbl = *vtbl;
	return z;
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

static int
dispatchSync(VtSession *z, Packet **pkt)
{
	(z->vtbl->sync)(z);
	if(packetSize(*pkt) != 0) {
		vtSetError(EProtocolBotch);
		return 0;
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
		case VtQSync:
			if(!dispatchSync(z, &p))
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

