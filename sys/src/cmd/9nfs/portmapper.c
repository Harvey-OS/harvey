/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * sunrpc portmapper
 */
#include "all.h"

typedef struct Portmap Portmap;
struct Portmap {
	int prog;
	int vers;
	int protocol;
	int port;
};

Portmap map[] = {
    100003, 2, IPPROTO_UDP, 2049, /* nfs v2 */
                                  //	100003, 3, IPPROTO_UDP, 2049,	/* nfs v3 */
    100005, 1, IPPROTO_UDP, 2049, /* mount */
    150001, 2, IPPROTO_UDP, 1111, /* pcnfsd v2 */
    150001, 1, IPPROTO_UDP, 1111, /* pcnfsd v1 */
    0,      0, 0,           0,
};

static void pmapinit(int, char**);
static int pmapnull(int, Rpccall*, Rpccall*);
static int pmapset(int, Rpccall*, Rpccall*);
static int pmapunset(int, Rpccall*, Rpccall*);
static int pmapgetport(int, Rpccall*, Rpccall*);
static int pmapdump(int, Rpccall*, Rpccall*);
static int pmapcallit(int, Rpccall*, Rpccall*);

static Procmap pmapproc[] = {0, pmapnull,    1, pmapset,  2, pmapunset,
                             3, pmapgetport, 4, pmapdump, 5, pmapcallit,
                             0, 0};

int myport = 111;

Progmap progmap[] = {
    100000, 2, pmapinit, pmapproc, 0, 0, 0,
};

void
main(int argc, char* argv[])
{
	server(argc, argv, myport, progmap);
}

static void
pmapinit(int argc, char** argv)
{
	ARGBEGIN
	{
	default:
		if(argopt(ARGC()) < 0)
			sysfatal("usage: %s %s", argv0, commonopts);
		break;
	}
	ARGEND;
	clog("portmapper init\n");
}

static int
pmapnull(int n, Rpccall* cmd, Rpccall* reply)
{
	USED(n);
	USED(cmd);
	USED(reply);
	return 0;
}

static int
pmapset(int n, Rpccall* cmd, Rpccall* reply)
{
	uint8_t* dataptr = reply->results;

	if(n != 16)
		return garbage(reply, "bad count");
	USED(cmd);
	PLONG(FALSE);
	return dataptr - (uint8_t*)reply->results;
}

static int
pmapunset(int n, Rpccall* cmd, Rpccall* reply)
{
	uint8_t* dataptr = reply->results;

	if(n != 16)
		return garbage(reply, "bad count");
	USED(cmd);
	PLONG(TRUE);
	return dataptr - (uint8_t*)reply->results;
}

static int
pmapgetport(int n, Rpccall* cmd, Rpccall* reply)
{
	int prog, vers, prot;
	uint8_t* argptr = cmd->args;
	uint8_t* dataptr = reply->results;
	Portmap* mp;

	clog("get port\n");

	if(n != 16)
		return garbage(reply, "bad count");
	prog = GLONG();
	vers = GLONG();
	prot = GLONG();
	chat("host=%I, port=%ld: ", cmd->host, cmd->port);
	chat("getport: %d, %d, %d...", prog, vers, prot);
	for(mp = map; mp->prog > 0; mp++)
		if(prog == mp->prog && vers == mp->vers && prot == mp->protocol)
			break;
	chat("%d\n", mp->port);
	PLONG(mp->port);
	return dataptr - (uint8_t*)reply->results;
}

static int
pmapdump(int n, Rpccall* cmd, Rpccall* reply)
{
	uint8_t* dataptr = reply->results;
	Portmap* mp;

	if(n != 0)
		return garbage(reply, "bad count");
	USED(cmd);
	for(mp = map; mp->prog > 0; mp++) {
		PLONG(1);
		PLONG(mp->prog);
		PLONG(mp->vers);
		PLONG(mp->protocol);
		PLONG(mp->port);
	}
	PLONG(0);
	return dataptr - (uint8_t*)reply->results;
}

static int
pmapcallit(int n, Rpccall* cmd, Rpccall* reply)
{
	int prog, vers, proc;
	uint8_t* argptr = cmd->args;
	uint8_t* dataptr = reply->results;
	Portmap* mp;

	if(n < 12)
		return garbage(reply, "bad count");
	prog = GLONG();
	vers = GLONG();
	proc = GLONG();
	chat("host=%I, port=%ld: ", cmd->host, cmd->port);
	chat("callit: %d, %d, %d...", prog, vers, proc);
	for(mp = map; mp->prog > 0; mp++)
		if(prog == mp->prog && vers == mp->vers && proc == 0)
			break;
	if(mp->port == 0) {
		chat("ignored\n");
		return -1;
	}
	chat("%d\n", mp->port);
	PLONG(mp->port);
	PLONG(0);
	return dataptr - (uint8_t*)reply->results;
}
