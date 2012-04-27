#include <u.h>
#include <libc.h>
#include <ip.h>
#include <thread.h>
#include "netbios.h"

void
nbnsmessagequestionfree(NbnsMessageQuestion **qp)
{
	NbnsMessageQuestion *q = *qp;
	if (q) {
		free(q);
		*qp = nil;
	}
}

void
nbnsmessageresourcefree(NbnsMessageResource **rp)
{
	NbnsMessageResource *r = *rp;
	if (r) {
		free(r->rdata);
		free(r);
		*rp = nil;
	}
}

static void
questionfree(NbnsMessageQuestion **qp)
{
	while (*qp) {
		NbnsMessageQuestion *next = (*qp)->next;
		nbnsmessagequestionfree(qp);
		*qp = next;
	}
}

static void
resourcefree(NbnsMessageResource **rp)
{
	while (*rp) {
		NbnsMessageResource *next = (*rp)->next;
		nbnsmessageresourcefree(rp);
		*rp = next;
	}
}

void
nbnsmessagefree(NbnsMessage **sp)
{
	NbnsMessage *s = *sp;
	if (s) {
		questionfree(&s->q);
		resourcefree(&s->an);
		resourcefree(&s->ns);
		resourcefree(&s->ar);
		free(s);
		*sp = nil;
	}
}

void
nbnsmessageaddquestion(NbnsMessage *s, NbnsMessageQuestion *q)
{
	NbnsMessageQuestion **qp;
	for (qp = &s->q; *qp; qp = &(*qp)->next)
		;
	*qp = q;
}

NbnsMessageQuestion *
nbnsmessagequestionnew(NbName name, ushort type, ushort class)
{
	NbnsMessageQuestion *q;
	q = mallocz(sizeof(*q), 1);
	if (q == nil)
		return nil;
	nbnamecpy(q->name, name);
	q->type = type;
	q->class = class;
	return q;
}

NbnsMessageResource *
nbnsmessageresourcenew(NbName name, ushort type, ushort class, ulong ttl, int rdlength, uchar *rdata)
{
	NbnsMessageResource *r;
	r= mallocz(sizeof(*r), 1);
	if (r == nil)
		return nil;
	nbnamecpy(r->name, name);
	r->type = type;
	r->class = class;
	r->ttl = ttl;
	r->rdlength = rdlength;
	if (rdlength) {
		r->rdata = malloc(rdlength);
		if (r->rdata == nil) {
			free(r);
			return nil;
		}
		memcpy(r->rdata, rdata, rdlength);
	}
	return r;
}

void
nbnsmessageaddresource(NbnsMessageResource **rp, NbnsMessageResource *r)
{
	for (; *rp; rp = &(*rp)->next)
		;
	*rp = r;
}

NbnsMessage *
nbnsmessagenew(void)
{
	return mallocz(sizeof(NbnsMessage), 1);
}

static int
resourcedecode(NbnsMessageResource **headp, int count, uchar *ap, uchar *pp, uchar *ep)
{
	uchar *p = pp;
	int i;
	for (i = 0; i < count; i++) {
		int n;
		NbnsMessageResource *r, **rp;
		r = mallocz(sizeof(NbnsMessageResource), 1);
		if (r == nil)
			return -1;
		for (rp = headp; *rp; rp = &(*rp)->next)
			;
		*rp = r;
		n = nbnamedecode(ap, p, ep, r->name);
		if (n == 0)
			return -1;
		p += n;
		if (p + 10 > ep)
			return -1;
		r->type = nhgets(p); p += 2;
		r->class = nhgets(p); p += 2;
		r->ttl = nhgetl(p); p += 4;
		r->rdlength = nhgets(p); p += 2;
//print("rdlength %d\n", r->rdlength);
		if (r->rdlength) {
			if (p + r->rdlength > ep)
				return -1;
			r->rdata = malloc(r->rdlength);
			if (r == nil)
				return -1;
			memcpy(r->rdata, p, r->rdlength);
			p += r->rdlength;
		}
	}
	return p - pp;
}

NbnsMessage *
nbnsconvM2S(uchar *ap, int nap)
{
	uchar *p, *ep;
	ushort qdcount, ancount, nscount, arcount, ctrl;
	int i;
	NbnsMessage *s;
	int n;

	if (nap < 12)
		return nil;
	p = ap;
	ep = ap + nap;
	s = nbnsmessagenew();
	if (s == nil)
		return nil;
	s->id = nhgets(p); p+= 2;
	ctrl = nhgets(p); p += 2;
	qdcount = nhgets(p); p += 2;
	ancount = nhgets(p); p += 2;
	nscount = nhgets(p); p += 2;
	arcount = nhgets(p); p += 2;
	s->response = (ctrl & NbnsResponse) != 0;
	s->opcode = (ctrl >> NbnsOpShift) & NbnsOpMask;
	s->broadcast = (ctrl & NbnsFlagBroadcast) != 0;
	s->recursionavailable = (ctrl & NbnsFlagRecursionAvailable) != 0;
	s->recursiondesired = (ctrl & NbnsFlagRecursionDesired) != 0;
	s->truncation = (ctrl & NbnsFlagTruncation) != 0;
	s->authoritativeanswer = (ctrl & NbnsFlagAuthoritativeAnswer) != 0;
	s->rcode = s->response ? (ctrl & NbnsRcodeMask) : 0;
	for (i = 0; i < qdcount; i++) {
		int n;
		NbName nbname;
		NbnsMessageQuestion *q;
		ushort type, class;
		n = nbnamedecode(ap, p, ep, nbname);
		if (n == 0)
			goto fail;
		p += n;
		if (p + 4 > ep)
			goto fail;
		type = nhgets(p); p += 2;
		class = nhgets(p); p += 2;
		q = nbnsmessagequestionnew(nbname, type, class);
		if (q == nil)
			goto fail;
		nbnsmessageaddquestion(s, q);
	}
	n = resourcedecode(&s->an, ancount, ap, p, ep);
	if (n < 0)
		goto fail;
	p += n;
	n = resourcedecode(&s->ns, nscount, ap, p, ep);
	if (n < 0)
		goto fail;
	p += n;
	n = resourcedecode(&s->ar, arcount, ap, p, ep);
	if (n < 0)
		goto fail;
//print("arcount %d\n", arcount);
	return s;
fail:
	nbnsmessagefree(&s);
	return nil;
}

static int
resourceencode(NbnsMessageResource *r, uchar *ap, uchar *ep)
{
	uchar *p = ap;
	for (; r; r = r->next) {
		int n = nbnameencode(p, ep, r->name);
		if (n == 0)
			return -1;
		p += n;
		if (p + 10 > ep)
			return -1;
		hnputs(p, r->type); p += 2;
		hnputs(p, r->class); p += 2;
		hnputl(p, r->ttl); p += 4;
		hnputs(p, r->rdlength); p += 2;
		if (p + r->rdlength > ep)
			return -1;
		memcpy(p, r->rdata, r->rdlength);
		p += r->rdlength;
	}
	return p - ap;
}

int
nbnsconvS2M(NbnsMessage *s, uchar *ap, int nap)
{
	uchar *p = ap;
	uchar *ep = ap + nap;
	ushort ctrl;
	NbnsMessageQuestion *q;
	NbnsMessageResource *r;
	int k;
	int n;

	if (p + 12 > ep)
		return 0;
	hnputs(p, s->id); p+= 2;
	ctrl = (s->opcode & NbnsOpMask) << NbnsOpShift;
	if (s->response) {
		ctrl |= s->rcode & NbnsRcodeMask;
		ctrl |= NbnsResponse;
	}
	if (s->broadcast)
		ctrl |= NbnsFlagBroadcast;
	if (s->recursionavailable)
		ctrl |= NbnsFlagRecursionAvailable;
	if (s->recursiondesired)
		ctrl |= NbnsFlagRecursionDesired;
	if (s->truncation)
		ctrl |= NbnsFlagTruncation;
	if (s->authoritativeanswer)
		ctrl |= NbnsFlagAuthoritativeAnswer;
	hnputs(p, ctrl); p += 2;
	for (q = s->q, k = 0; q; k++, q = q->next)
		;
	hnputs(p, k); p += 2;
	for (r = s->an, k = 0; r; k++, r = r->next)
		;
	hnputs(p, k); p += 2;
	for (r = s->ns, k = 0; r; k++, r = r->next)
		;
	hnputs(p, k); p += 2;
	for (r = s->ar, k = 0; r; k++, r = r->next)
		;
	hnputs(p, k); p += 2;
	for (q = s->q; q; q = q->next) {
		int n = nbnameencode(p, ep, q->name);
		if (n == 0)
			return 0;
		p += n;
		if (p + 4 > ep)
			return 0;
		hnputs(p, q->type); p += 2;
		hnputs(p, q->class); p += 2;
	}
	n = resourceencode(s->an, p, ep);
	if (n < 0)
		return 0;
	p += n;
	n = resourceencode(s->ns, p, ep);
	if (n < 0)
		return 0;
	p += n;
	n = resourceencode(s->ar, p, ep);
	if (n < 0)
		return 0;
	p += n;
	return p - ap;
}

