#include <u.h>
#include <libc.h>
#include <thread.h>
#include "le.h"
#include "usb.h"
#include "usbfs.h"
#include "ccid.h"
#include "eprpc.h"
#include "tagrd.h"

int
matchatr(uchar *atr, uchar *atr2, int sz)
{
	return (memcmp(atr, atr2, sz) == 0);
}

int
matchcard(Ccid *ccid, uchar *atr, int atrsz)
{
	int ncmp;

	if(atrsz > ccid->sl->natr)
		ncmp = ccid->sl->natr;
	else
		ncmp = atrsz;
	return matchatr(atr, ccid->sl->atr, ncmp);
}

Client *
matchclient(Ccid *ccid, uchar *atr, int atrsz)
{
	int i;
	int ncmp;

	for(i = 0; i < Maxcli; i++){
		if(ccid->cl[i] != nil){ 
			if(atrsz > ccid->cl[i]->natr)
				ncmp = ccid->cl[i]->natr;
			else
				ncmp = atrsz;
			if(matchatr(atr, ccid->cl[i]->atr, ncmp))
				break;
		}
	}
	if(i == Maxcli)
		return nil;
	return ccid->cl[i];
}

Client *
fillclient(Client *cl, uchar *atr, int atrsz)
{
	cl->atr = emallocz(atrsz, 1);
	cl->natr = atrsz;
	memmove(cl->atr, atr, atrsz);
	cl->to = chancreate(sizeof(Cmsg *), 1);
	cl->from = chancreate(sizeof(Cmsg *), 1);
	cl->start = chancreate(sizeof(void *), 1);
	proccreate(clientproc, cl, Stacksz);

	return cl;
}

static void
freeclient(Client *cl)
{
	qlock(cl->ccid);
	cl->ccid->cl[cl->ncli] = nil;
	qunlock(cl->ccid);
	chanfree(cl->to);
	cl->to = nil;
	chanfree(cl->from);
	cl->from = nil;
	cl->sl = nil;
	free(cl);
}

static void
cancellastop(Client *cl)
{
	Cmsg *c;

	dcprint(2, "ccid: maybe cancelling last operation...\n");
	ccidabort(cl->ccid, cl->sl->nreq, 0, nil);
	c = iccmsgrpc(cl->ccid, nil, 0, AbortTyp, 0);
	free(c);
}

 /*	BUG: this should probably be a thread and not do I/O
 *	
 */
void
clientproc(void *u)
{
	Client *cl;
	Cmsg *c, *rspc;
	Alt a[] = {
	/*	 c		v		op   */
		{nil,	&c,	CHANRCV},
		{nil,	&rspc,	CHANSND},
		{nil,	nil,	CHANEND},
	};
	cl = u;
	threadsetname("client");


	/* nil means go away */
	while(1){
		c = recvp(cl->to);
		if(c == nil)
			break;
		qlock(cl->ccid);
		rspc = iccmsgrpc(cl->ccid, c->data, c->len, XfrBlockTyp, 0);
		if(rspc == nil ){
			rspc = mallocz(sizeof *rspc, 1);
			rspc->errstr = smprint("%r");
			rspc->len = -1;
		}
		if(c->errstr == nil)
			dcprint(2, "ccid: received %lud\n", rspc->len);
		else
			dcprint(2, "ccid: err %s\n", rspc->errstr);
		qunlock(cl->ccid);
		
		a[0].c = cl->to;
		a[1].c = cl->from;
		switch(alt(a)){
		case 0:	/* cancel */
			if(c->isabort == 0){
				fprint(2, "protocol botch: strange non abortl");
				continue;
			}
			else{
				qlock(cl->ccid);
				free(rspc);
				if(rspc != nil){
					cancellastop(cl);
				}
				qunlock(cl->ccid);
			}
			break;
		case 1:	/* sent response */
			dcprint(2, "ccid: client proc sent response \n");
			break;
		default:
			sysfatal("can't happen");
		}
	}

	freeclient(cl);
}
