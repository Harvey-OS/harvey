#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <thread.h>
#include <worker.h>
#include "le.h"
#include "usb.h"
#include "usbfs.h"
#include "ccid.h"
#include "eprpc.h"
#include "atr.h"
#include "tagrd.h"

typedef struct CcIntr CcIntr;
struct CcIntr{
	Channel *c;
	Ccid *ccid;
};

enum{
	SEC = 1000000000LL,	/* in NSEC*/
	EVOK = 0,
	CPRES = 1,
	CNOTPRES = 2,
	INTRERR = 0xff,
	EXIT = 0xfe,
};

void
intrproc(void *u)
{
	CcIntr *ci;
	ulong v;
	Ccid *ccid;
	int dfd, res, nr;
	uchar buf[16];
	Imsg im;

	threadsetname("intrproc");

	ci = u;
	ccid = ci->ccid;
	qlock(ccid);
	dfd = ccid->epintr->dfd;
	qunlock(ccid);


	for(;;){
		nr = read(dfd, buf, sizeof buf);
		if(nr < 0)
			sendul(ci->c, INTRERR);
		else{
			res = unpackimsg(&im, buf, nr);
			if(res < 0){
				fprint(2, "ccidial: unpacking msg %r\n");
				continue;
			}
			if(im.type == HardwareErrorTypI){
				fprint(2, "ccidial: hardware error %#2.2ux\n", im.type);
				sendul(ci->c, INTRERR);
			}
			else if( im.type == NotifySlotChangeTypI) {
				dcprint(2, "ccidial: slot change: %#2.2lx\n", im.sloticcstate);
				if(im.sloticcstate & NoIccpresent == 0)
					sendul(ci->c, CPRES);
				else
					sendul(ci->c, CNOTPRES);
			}
			else{
				fprint(2, "ccidial: unknown slot message %#2.2ux\n", im.type);
				continue; /* no send, no receive */
			}
		}
		v = recvul(ci->c);
		if(v == EXIT)
			break;
	}
	chanfree(ci->c);
	free(ci);
}

/*	I need a timer because intr is optional... and sometimes they don't tell you
 *	BUG: I will set polling to 10 sec for debugging, though it should be less
 */
void
statusreader(void *u)
{
	
	int r, ispres, intr;
	Ccid *ccid;
	CcIntr *ci;
	Channel *c;
	ulong v;

	
	threadsetname("statusreaderproc");

	ccid = u;
	v = EVOK;

	/* chancreate anyway, if no proc is created free it myself */
	c = chancreate(sizeof(v), 0);	/* BUG, chancreate fails */
	if(ccid->hasintr){
		ci = mallocz(sizeof (CcIntr), 1);	/* BUG  malloc fails?*/
		ci->c = c;
		ci->ccid = ccid;
		proccreate(intrproc, ci, Stacksz);
	}

	for(;;){
		intr = 0;
		ispres = 0;
		r = recvt(c, &v, nsec()+2*SEC);
		if(r == 1){
			intr = 1;
			if(v == CPRES)
				ispres = 1;
			else if(v == CNOTPRES)
				ispres = 0;
			else if(ccidrecover(ccid, "reading from intr") < 0){
				fprint(2, "ccid: fatal error reading from intr");
				sendul(c, EXIT);
				break;
			}
		}
		else if( r == 0)	/* timed out */
				dcprint(2, "ccid: timeout, polling myself\n");
		else
			sysfatal("should not happen r:  %d", r);
	
		qlock(ccid);
		if(!ispres)
			ispres = isiccpresent(ccid);
		dcprint(2, "ccid: ccid->sl: %p, ispres: %d\n", ccid->sl, ispres);
		if(ispres){
			if(ccid->sl == nil)
				ccidslotinit(ccid, 0); /* BUG only 0 supported */
		}
		else if( ccid->sl != nil )
			ccidslotfree(ccid, 0);
		qunlock(ccid);
		if(intr)
			sendul(c, EVOK);
	}
	closedev(ccid->dev);
	if(ccid->hasintr == 0){
		chanfree(c);
	}

}
