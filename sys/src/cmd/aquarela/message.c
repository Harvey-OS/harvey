#include <u.h>
#include <libc.h>
#include <ip.h>
#include <thread.h>
#include "netbios.h"

NbnsMessage *
nbnsmessagenamequeryrequestnew(ushort id, int broadcast, NbName name)
{
	NbnsMessage *s;
	NbnsMessageQuestion *q;
	s = nbnsmessagenew();
	if (s == nil)
		return nil;
	s->id = id;
	s->opcode = NbnsOpQuery;
	s->broadcast = broadcast;
	s->recursiondesired = 1;
	q =  nbnsmessagequestionnew(name, NbnsQuestionTypeNb, NbnsQuestionClassIn);
	if (q == nil) {
		nbnsmessagefree(&s);
		return nil;
	}
	nbnsmessageaddquestion(s, q);
	return s;
}

NbnsMessage *
nbnsmessagenameregistrationrequestnew(ushort id, int broadcast, NbName name, ulong ttl, uchar *ipaddr)
{
	NbnsMessage *s;
	NbnsMessageQuestion *q;
	uchar rdata[6];
	NbnsMessageResource *r;

	s = nbnsmessagenew();
	if (s == nil)
		return nil;
	s->id = id;
	s->opcode = NbnsOpRegistration;
	s->broadcast = broadcast;
	s->recursiondesired = 1;
	q =  nbnsmessagequestionnew(name, NbnsQuestionTypeNb, NbnsQuestionClassIn);
	if (q == nil) {
		nbnsmessagefree(&s);
		return nil;
	}
	nbnsmessageaddquestion(s, q);
	rdata[0] = 0;
	rdata[1] = 0;
	v6tov4(rdata + 2, ipaddr);
	r = nbnsmessageresourcenew(name, NbnsResourceTypeNb, NbnsResourceClassIn, ttl, 6, rdata);
	nbnsmessageaddresource(&s->ar, r);
	return s;
}
