#include <u.h>
#include <libc.h>
#include <ip.h>
#include <thread.h>
#include "netbios.h"

int
nbnsaddname(uchar *serveripaddr, NbName name, ulong ttl, uchar *ipaddr)
{
	NbnsMessage *nq;
	Alt aa[3];
	int tries = NbnsRetryBroadcast;
	NbnsAlarm *a;
	int rv;
	NbnsMessage *response;

	nq = nbnsmessagenameregistrationrequestnew(0, serveripaddr == nil, name, ttl, ipaddr);
	if (nq == nil)
		return -1;
	a = nbnsalarmnew();
	if (a == nil) {
		free(nq);
		return -1;
	}
	aa[0].c = a->c;
	aa[0].v = nil;
	aa[0].op = CHANRCV;
	aa[1].op = CHANRCV;
	aa[2].op = CHANEND;
	while (tries > 0) {
		NbnsTransaction *t;
		nq->id = nbnsnextid();
		t = nbnstransactionnew(nq, serveripaddr);
		aa[1].c = t->c;
		aa[1].v = &response;
		nbnsalarmset(a, NbnsTimeoutBroadcast);
		for (;;) {
			int i;
			i = alt(aa);
			if (i == 0) {
				tries--;
				break;
			}
			else if (i == 1) {
				if (response->opcode == NbnsOpRegistration) {
					nbnstransactionfree(&t);
					goto done;
				}
				nbnsmessagefree(&response);
			}
		}
		nbnstransactionfree(&t);
	}
done:
	if (tries == 0)
		rv = -1;
	else {
		if (response->rcode != 0)
			rv = response->rcode;
		else if (response->an == nil)
			rv = -1;
		else
			rv = 0;
		nbnsmessagefree(&response);
	}
	nbnsalarmfree(&a);
	nbnsmessagefree(&nq);
	return rv;
}
