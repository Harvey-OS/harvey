#include <u.h>
#include <libc.h>
#include <ip.h>
#include <thread.h>
#include "netbios.h"

static char *
opname(int opcode)
{
	switch (opcode) {
	case NbnsOpQuery: return "query";
	case NbnsOpRegistration: return "registration";
	case NbnsOpRelease: return "release";
	case NbnsOpWack: return "wack";
	case NbnsOpRefresh: return "refresh";
	default:
		return "???";
	}
}

void
nbnsdumpname(NbName name)
{
	int x;
	for (x = 0; x < NbNameLen - 1; x++) {
		if (name[x] == ' ')
			break;
		print("%c", tolower(name[x]));
	}
	print("\\x%.2ux", name[NbNameLen - 1]);
}

void
nbnsdumpmessagequestion(NbnsMessageQuestion *q)
{
	print("question: ");
	nbnsdumpname(q->name);
	switch (q->type) {
	case NbnsQuestionTypeNb: print(" NB");	break;
	case NbnsQuestionTypeNbStat: print(" NBSTAT"); break;
	default: print(" ???");
	}
	switch (q->class) {
	case NbnsQuestionClassIn: print(" IN"); break;
	default: print(" ???");
	}
	print("\n");
}

void
nbnsdumpmessageresource(NbnsMessageResource *r, char *name)
{
	print("%s: ", name);
	nbnsdumpname(r->name);
	switch (r->type) {
	case NbnsResourceTypeA: print(" A");	break;
	case NbnsResourceTypeNs: print(" NS");	break;
	case NbnsResourceTypeNull: print(" NULL");	break;
	case NbnsResourceTypeNb: print(" NB");	break;
	case NbnsResourceTypeNbStat: print(" NBSTAT"); break;
	default: print(" ???");
	}
	switch (r->class) {
	case NbnsResourceClassIn: print(" IN"); break;
	default: print(" ???");
	}
	print(" ttl: %lud", r->ttl);
	if (r->rdlength) {
		int i;
		print(" rdata: ");
		for (i = 0; i < r->rdlength; i++)
			print("%.2ux", r->rdata[i]);
	}
	print("\n");
}

void
nbnsdumpmessage(NbnsMessage *s)
{
	NbnsMessageQuestion *q;
	NbnsMessageResource *r;
	print("0x%.4ux %s %s (%d)",
		s->id, opname(s->opcode), s->response ? "response" : "request", s->opcode);
	if (s->broadcast)
		print(" B");
	if (s->recursionavailable)
		print(" RA");
	if (s->recursiondesired)
		print(" RD");
	if (s->truncation)
		print(" TC");
	if (s->authoritativeanswer)
		print(" AA");
	if (s->response)
		print(" rcode %d", s->rcode);
	print("\n");
	for (q = s->q; q; q = q->next)
		nbnsdumpmessagequestion(q);
	for (r = s->an; r; r = r->next)
		nbnsdumpmessageresource(r, "answer");
	for (r = s->ns; r; r = r->next)
		nbnsdumpmessageresource(r, "ns");
	for (r = s->ar; r; r = r->next)
		nbnsdumpmessageresource(r, "additional");
}

void
nbdumpdata(void *ap, long n)
{
	uchar *p = ap;
	long i;
	i = 0;
	while (i < n) {
		int l = n - i < 16 ? n - i : 16;
		int b;
		print("0x%.4lux  ", i);
		for (b = 0; b < l; b += 2) {
			print(" %.2ux", p[i + b]);
			if (b < l - 1)
				print("%.2ux", p[i + b + 1]);
			else
				print("  ");
		}
		while (b < 16) {
			print("     ");
			b++;
		}
		print("        ");
		for (b = 0; b < l; b++)
			if (p[i + b] >= ' ' && p[i + b] <= '~')
				print("%c", p[i + b]);
			else
				print(".");
		print("\n");
		i += l;
	}
}
