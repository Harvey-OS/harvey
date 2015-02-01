/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct VtAuth VtAuth;

/* op codes */
enum {
	VtRError = 1,
	VtQPing,
	VtRPing,
	VtQHello,
	VtRHello,
	VtQGoodbye,
	VtRGoodbye,	/* not used */
	VtQAuth0,
	VtRAuth0,
	VtQAuth1,
	VtRAuth1,
	VtQRead,
	VtRRead,
	VtQWrite,
	VtRWrite,
	VtQSync,
	VtRSync,

	VtMaxOp
};

/* connection state */
enum {
	VtStateAlloc,
	VtStateConnected,
	VtStateClosed,
};

/* auth state */
enum {
	VtAuthHello,
	VtAuth0,
	VtAuth1,
	VtAuthOK,
	VtAuthFailed,
};

struct VtAuth {
	int state;
	uchar client[VtScoreSize];
	uchar sever[VtScoreSize];
};

struct VtSession {
	VtLock *lk;
	VtServerVtbl *vtbl;	/* == nil means client side */
	int cstate;		/* connection state */
	int fd;
	char fderror[ERRMAX];

	VtAuth auth;

	VtSha1 *inHash;
	VtLock *inLock;
	Packet *part;		/* partial packet */

	VtSha1 *outHash;
	VtLock *outLock;

	int debug;
	int version;
	int ref;
	char *uid;
	char *sid;
	int cryptoStrength;
	int compression;
	int crypto;
	int codec;
};

