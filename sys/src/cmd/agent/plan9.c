#include "agent.h"

typedef struct Netkey Netkey;
struct Netkey {
	Key *k;
	char key[DESKEYLEN];
	char response[32];
};

void*
netkeyopen(Key *k, char*, int client)
{
	Netkey *nk;

	if((k->flags & Fconfirmopen) && confirm("open netkey") < 0)
		return nil;

	assert(client);
	nk = emalloc(sizeof(Netkey));
	nk->k = k;
	passtokey(nk->key, s_to_c(k->data));
	return nk;
}

void
netkeyclose(void *a)
{
	memset(a, 0xFF, sizeof(Netkey));
	free(a);
}

int
netkeywrite(void *a, void *wbuf, int n)
{
	Netkey *nk;
	char buf[32];

	nk = a;
	if((nk->k->flags & Fconfirmuse) && confirm("use netkey") < 0)
		return -1;

	if(n > 31)
		n = 31;
	strncpy(buf, wbuf, n);
	buf[n] = '\0';
	sprint(buf, "%lud", strtol(buf, 0, 10));
	netcrypt(nk->key, buf);
	strcpy(nk->response, buf);
	return n;
}

int
netkeyread(void *a, void *buf, int n)
{
	Netkey *nk;

	nk = a;
	if(n > strlen(nk->response))
		n = strlen(nk->response);
	memmove(buf, nk->response, n);
	return n;
}

Proto netkey = {
.name=	"netkey",
.read=	netkeyread,
.write=	netkeywrite,
.close=	netkeyclose,
.open=	netkeyopen,
.perm=	0666,
};

Proto plan9 = {
.name=	"plan9",		/* BUG */
.read=	netkeyread,
.write=	netkeywrite,
.close=	netkeyclose,
.open=	netkeyopen,
.perm=	0666,
};

