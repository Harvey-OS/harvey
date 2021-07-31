#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <String.h>
#include "dat.h"

#include <libsec.h>

typedef struct Apopstate Apopstate;
struct Apopstate {
	Key *k;
	char resp[64];
};

static int
md5conv(va_list *va, Fconv *fp)
{
	char buf[MD5dlen*2+1];
	uchar *p;
	int i;

	p = va_arg(*va, uchar*);
	for(i=0; i<MD5dlen; i++)
		sprint(buf+2*i, "%.2ux", p[i]);
	strconv(buf, fp);
	return 0;
}

void*
apopopen(Key *k, char*, int client)
{
	Apopstate *a;

	assert(client);
	a = emalloc(sizeof *a);
	a->k = k;
	return a;
}

int
apopwrite(void *va, void *buf, int n)
{
	int i;
	uchar digest[MD5dlen];
	Apopstate *s;
	DigestState *md5s;

	s = va;
	md5s = md5(buf, n, nil, nil);
	md5((uchar*)s_to_c(s->k->data), s_len(s->k->data), nil, md5s);
	md5(nil, 0, digest, md5s);

	for(i=0; i<MD5dlen; i++)
		sprint(s->resp+2*i, "%.2ux", digest[i]);
	return n;
}

int
apopread(void *va, void *buf, int n)
{
	Apopstate *s;

	s = va;
	if(n > strlen(s->resp))
		n = strlen(s->resp);
	memmove(buf, s->resp, n);
	return n;
}

void
apopclose(void *v)
{
	free(v);
}

Proto apop = {
.name=	"apop",
.perm=	0666,
.open=	apopopen,
.write=	apopwrite,
.read=	apopread,
.close=	apopclose,
};
