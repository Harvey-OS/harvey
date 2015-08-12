/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include <sunrpc.h>

/*
 * RPC protocol constants
 */
enum { RpcVersion = 2,

       /* msg type */
       MsgCall = 0,
       MsgReply = 1,

       /* reply stat */
       MsgAccepted = 0,
       MsgDenied = 1,

       /* accept stat */
       MsgSuccess = 0,
       MsgProgUnavail = 1,
       MsgProgMismatch = 2,
       MsgProcUnavail = 3,
       MsgGarbageArgs = 4,
       MsgSystemErr = 5,

       /* reject stat */
       MsgRpcMismatch = 0,
       MsgAuthError = 1,

       /* msg auth xxx */
       MsgAuthOk = 0,
       MsgAuthBadCred = 1,
       MsgAuthRejectedCred = 2,
       MsgAuthBadVerf = 3,
       MsgAuthRejectedVerf = 4,
       MsgAuthTooWeak = 5,
       MsgAuthInvalidResp = 6,
       MsgAuthFailed = 7,
};

SunStatus
sunRpcPack(uint8_t* a, uint8_t* ea, uint8_t** pa, SunRpc* rpc)
{
	uint32_t x;

	if(sunUint32Pack(a, ea, &a, &rpc->xid) < 0)
		goto Err;
	if(rpc->iscall) {
		if(sunUint32Pack(a, ea, &a, (x = MsgCall, &x)) < 0 ||
		   sunUint32Pack(a, ea, &a, (x = RpcVersion, &x)) < 0 ||
		   sunUint32Pack(a, ea, &a, &rpc->prog) < 0 ||
		   sunUint32Pack(a, ea, &a, &rpc->vers) < 0 ||
		   sunUint32Pack(a, ea, &a, &rpc->proc) < 0 ||
		   sunAuthInfoPack(a, ea, &a, &rpc->cred) < 0 ||
		   sunAuthInfoPack(a, ea, &a, &rpc->verf) < 0 ||
		   sunFixedOpaquePack(a, ea, &a, rpc->data, rpc->ndata) < 0)
			goto Err;
	} else {
		if(sunUint32Pack(a, ea, &a, (x = MsgReply, &x)) < 0)
			goto Err;
		switch(rpc->status & 0xF0000) {
		case 0:
		case SunAcceptError:
			if(sunUint32Pack(a, ea, &a, (x = MsgAccepted, &x)) <
			       0 ||
			   sunAuthInfoPack(a, ea, &a, &rpc->verf) < 0)
				goto Err;
			break;
		default:
			if(sunUint32Pack(a, ea, &a, (x = MsgDenied, &x)) < 0)
				goto Err;
			break;
		}

		switch(rpc->status) {
		case SunSuccess:
			if(sunUint32Pack(a, ea, &a, (x = MsgSuccess, &x)) < 0 ||
			   sunFixedOpaquePack(a, ea, &a, rpc->data,
			                      rpc->ndata) < 0)
				goto Err;
			break;
		case SunRpcMismatch:
		case SunProgMismatch:
			if(sunUint32Pack(a, ea, &a,
			                 (x = rpc->status & 0xFFFF, &x)) < 0 ||
			   sunUint32Pack(a, ea, &a, &rpc->low) < 0 ||
			   sunUint32Pack(a, ea, &a, &rpc->high) < 0)
				goto Err;
			break;
		default:
			if(sunUint32Pack(a, ea, &a,
			                 (x = rpc->status & 0xFFFF, &x)) < 0)
				goto Err;
			break;
		}
	}
	*pa = a;
	return SunSuccess;

Err:
	*pa = ea;
	return SunGarbageArgs;
}

uint
sunRpcSize(SunRpc* rpc)
{
	uint a;

	a = 4;
	if(rpc->iscall) {
		a += 5 * 4;
		a += sunAuthInfoSize(&rpc->cred);
		a += sunAuthInfoSize(&rpc->verf);
		a += sunFixedOpaqueSize(rpc->ndata);
	} else {
		a += 4;
		switch(rpc->status & 0xF0000) {
		case 0:
		case SunAcceptError:
			a += 4 + sunAuthInfoSize(&rpc->verf);
			break;
		default:
			a += 4;
			break;
		}

		switch(rpc->status) {
		case SunSuccess:
			a += 4 + sunFixedOpaqueSize(rpc->ndata);
			break;
		case SunRpcMismatch:
		case SunProgMismatch:
			a += 3 * 4;
		default:
			a += 4;
		}
	}
	return a;
}

SunStatus
sunRpcUnpack(uint8_t* a, uint8_t* ea, uint8_t** pa, SunRpc* rpc)
{
	uint32_t x;

	memset(rpc, 0, sizeof *rpc);
	if(sunUint32Unpack(a, ea, &a, &rpc->xid) < 0 ||
	   sunUint32Unpack(a, ea, &a, &x) < 0)
		goto Err;

	switch(x) {
	default:
		goto Err;
	case MsgCall:
		rpc->iscall = 1;
		if(sunUint32Unpack(a, ea, &a, &x) < 0 || x != RpcVersion ||
		   sunUint32Unpack(a, ea, &a, &rpc->prog) < 0 ||
		   sunUint32Unpack(a, ea, &a, &rpc->vers) < 0 ||
		   sunUint32Unpack(a, ea, &a, &rpc->proc) < 0 ||
		   sunAuthInfoUnpack(a, ea, &a, &rpc->cred) < 0 ||
		   sunAuthInfoUnpack(a, ea, &a, &rpc->verf) < 0)
			goto Err;
		rpc->ndata = ea - a;
		rpc->data = a;
		a = ea;
		break;

	case MsgReply:
		rpc->iscall = 0;
		if(sunUint32Unpack(a, ea, &a, &x) < 0)
			goto Err;
		switch(x) {
		default:
			goto Err;
		case MsgAccepted:
			if(sunAuthInfoUnpack(a, ea, &a, &rpc->verf) < 0 ||
			   sunUint32Unpack(a, ea, &a, &x) < 0)
				goto Err;
			switch(x) {
			case MsgSuccess:
				rpc->status = SunSuccess;
				rpc->ndata = ea - a;
				rpc->data = a;
				a = ea;
				break;
			case MsgProgUnavail:
			case MsgProcUnavail:
			case MsgGarbageArgs:
			case MsgSystemErr:
				rpc->status = SunAcceptError | x;
				break;
			case MsgProgMismatch:
				rpc->status = SunAcceptError | x;
				if(sunUint32Unpack(a, ea, &a, &rpc->low) < 0 ||
				   sunUint32Unpack(a, ea, &a, &rpc->high) < 0)
					goto Err;
				break;
			}
			break;
		case MsgDenied:
			if(sunUint32Unpack(a, ea, &a, &x) < 0)
				goto Err;
			switch(x) {
			default:
				goto Err;
			case MsgAuthError:
				if(sunUint32Unpack(a, ea, &a, &x) < 0)
					goto Err;
				rpc->status = SunAuthError | x;
				break;
			case MsgRpcMismatch:
				rpc->status = SunRejectError | x;
				if(sunUint32Unpack(a, ea, &a, &rpc->low) < 0 ||
				   sunUint32Unpack(a, ea, &a, &rpc->high) < 0)
					goto Err;
				break;
			}
			break;
		}
	}
	*pa = a;
	return SunSuccess;

Err:
	*pa = ea;
	return SunGarbageArgs;
}

void
sunRpcPrint(Fmt* fmt, SunRpc* rpc)
{
	fmtprint(fmt, "xid=%#ux", rpc->xid);
	if(rpc->iscall) {
		fmtprint(fmt, " prog %#ux vers %#ux proc %#ux [", rpc->prog,
		         rpc->vers, rpc->proc);
		sunAuthInfoPrint(fmt, &rpc->cred);
		fmtprint(fmt, "] [");
		sunAuthInfoPrint(fmt, &rpc->verf);
		fmtprint(fmt, "]");
	} else {
		fmtprint(fmt, " status %#ux [", rpc->status);
		sunAuthInfoPrint(fmt, &rpc->verf);
		fmtprint(fmt, "] low %#ux high %#ux", rpc->low, rpc->high);
	}
}

void
sunAuthInfoPrint(Fmt* fmt, SunAuthInfo* ai)
{
	switch(ai->flavor) {
	case SunAuthNone:
		fmtprint(fmt, "none");
		break;
	case SunAuthShort:
		fmtprint(fmt, "short");
		break;
	case SunAuthSys:
		fmtprint(fmt, "sys");
		break;
	default:
		fmtprint(fmt, "%#ux", ai->flavor);
		break;
	}
	//	if(ai->ndata)
	//		fmtprint(fmt, " %.*H", ai->ndata, ai->data);
}

uint
sunAuthInfoSize(SunAuthInfo* ai)
{
	return 4 + sunVarOpaqueSize(ai->ndata);
}

int
sunAuthInfoPack(uint8_t* a, uint8_t* ea, uint8_t** pa, SunAuthInfo* ai)
{
	if(sunUint32Pack(a, ea, &a, &ai->flavor) < 0 ||
	   sunVarOpaquePack(a, ea, &a, &ai->data, &ai->ndata, 400) < 0)
		goto Err;
	*pa = a;
	return 0;

Err:
	*pa = ea;
	return -1;
}

int
sunAuthInfoUnpack(uint8_t* a, uint8_t* ea, uint8_t** pa, SunAuthInfo* ai)
{
	if(sunUint32Unpack(a, ea, &a, &ai->flavor) < 0 ||
	   sunVarOpaqueUnpack(a, ea, &a, &ai->data, &ai->ndata, 400) < 0)
		goto Err;
	*pa = a;
	return 0;

Err:
	*pa = ea;
	return -1;
}

int
sunEnumPack(uint8_t* a, uint8_t* ea, uint8_t** pa, int* e)
{
	uint32_t x;

	x = *e;
	return sunUint32Pack(a, ea, pa, &x);
}

int
sunUint1Pack(uint8_t* a, uint8_t* ea, uint8_t** pa, u1int* u)
{
	uint32_t x;

	x = *u;
	return sunUint32Pack(a, ea, pa, &x);
}

int
sunUint32Pack(uint8_t* a, uint8_t* ea, uint8_t** pa, uint32_t* u)
{
	uint32_t x;

	if(ea - a < 4)
		goto Err;

	x = *u;
	*a++ = x >> 24;
	*a++ = x >> 16;
	*a++ = x >> 8;
	*a++ = x;
	*pa = a;
	return 0;

Err:
	*pa = ea;
	return -1;
}

int
sunEnumUnpack(uint8_t* a, uint8_t* ea, uint8_t** pa, int* e)
{
	uint32_t x;
	if(sunUint32Unpack(a, ea, pa, &x) < 0)
		return -1;
	*e = x;
	return 0;
}

int
sunUint1Unpack(uint8_t* a, uint8_t* ea, uint8_t** pa, u1int* u)
{
	uint32_t x;
	if(sunUint32Unpack(a, ea, pa, &x) < 0 || (x != 0 && x != 1)) {
		*pa = ea;
		return -1;
	}
	*u = x;
	return 0;
}

int
sunUint32Unpack(uint8_t* a, uint8_t* ea, uint8_t** pa, uint32_t* u)
{
	uint32_t x;

	if(ea - a < 4)
		goto Err;
	x = *a++ << 24;
	x |= *a++ << 16;
	x |= *a++ << 8;
	x |= *a++;
	*pa = a;
	*u = x;
	return 0;

Err:
	*pa = ea;
	return -1;
}

int
sunUint64Unpack(uint8_t* a, uint8_t* ea, uint8_t** pa, uint64_t* u)
{
	uint32_t x, y;

	if(sunUint32Unpack(a, ea, &a, &x) < 0 ||
	   sunUint32Unpack(a, ea, &a, &y) < 0)
		goto Err;
	*u = ((uint64_t)x << 32) | y;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}

int
sunUint64Pack(uint8_t* a, uint8_t* ea, uint8_t** pa, uint64_t* u)
{
	uint32_t x, y;

	x = *u >> 32;
	y = *u;
	if(sunUint32Pack(a, ea, &a, &x) < 0 || sunUint32Pack(a, ea, &a, &y) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}

uint
sunStringSize(char* s)
{
	return (4 + strlen(s) + 3) & ~3;
}

int
sunStringUnpack(uint8_t* a, uint8_t* ea, uint8_t** pa, char** s, uint32_t max)
{
	uint8_t* dat;
	uint32_t n;

	if(sunVarOpaqueUnpack(a, ea, pa, &dat, &n, max) < 0)
		goto Err;
	/* slide string down over length to make room for NUL */
	memmove(dat - 1, dat, n);
	dat[-1 + n] = 0;
	*s = (char*)(dat - 1);
	return 0;
Err:
	return -1;
}

int
sunStringPack(uint8_t* a, uint8_t* ea, uint8_t** pa, char** s, uint32_t max)
{
	uint32_t n;

	n = strlen(*s);
	return sunVarOpaquePack(a, ea, pa, (uint8_t**)s, &n, max);
}

uint
sunVarOpaqueSize(uint32_t n)
{
	return (4 + n + 3) & ~3;
}

int
sunVarOpaquePack(uint8_t* a, uint8_t* ea, uint8_t** pa, uint8_t** dat,
                 uint32_t* ndat, uint32_t max)
{
	if(*ndat > max || sunUint32Pack(a, ea, &a, ndat) < 0 ||
	   sunFixedOpaquePack(a, ea, &a, *dat, *ndat) < 0)
		goto Err;
	*pa = a;
	return 0;

Err:
	*pa = ea;
	return -1;
}

int
sunVarOpaqueUnpack(uint8_t* a, uint8_t* ea, uint8_t** pa, uint8_t** dat,
                   uint32_t* ndat, uint32_t max)
{
	if(sunUint32Unpack(a, ea, &a, ndat) < 0 || *ndat > max)
		goto Err;
	*dat = a;
	a += (*ndat + 3) & ~3;
	if(a > ea)
		goto Err;
	*pa = a;
	return 0;

Err:
	*pa = ea;
	return -1;
}

uint
sunFixedOpaqueSize(uint32_t n)
{
	return (n + 3) & ~3;
}

int
sunFixedOpaquePack(uint8_t* a, uint8_t* ea, uint8_t** pa, uint8_t* dat,
                   uint32_t n)
{
	uint nn;

	nn = (n + 3) & ~3;
	if(a + nn > ea)
		goto Err;
	memmove(a, dat, n);
	if(nn > n)
		memset(a + n, 0, nn - n);
	a += nn;
	*pa = a;
	return 0;

Err:
	*pa = ea;
	return -1;
}

int
sunFixedOpaqueUnpack(uint8_t* a, uint8_t* ea, uint8_t** pa, uint8_t* dat,
                     uint32_t n)
{
	uint nn;

	nn = (n + 3) & ~3;
	if(a + nn > ea)
		goto Err;
	memmove(dat, a, n);
	a += nn;
	*pa = a;
	return 0;

Err:
	*pa = ea;
	return -1;
}
