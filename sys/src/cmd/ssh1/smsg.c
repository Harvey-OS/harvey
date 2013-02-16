#include "ssh.h"
#include <bio.h>

static void
send_ssh_smsg_public_key(Conn *c)
{
	int i;
	Msg *m;

	m = allocmsg(c, SSH_SMSG_PUBLIC_KEY, 2048);
	putbytes(m, c->cookie, COOKIELEN);
	putRSApub(m, c->serverkey);
	putRSApub(m, c->hostkey);
	putlong(m, c->flags);

	for(i=0; i<c->nokcipher; i++)
		c->ciphermask |= 1<<c->okcipher[i]->id;
	putlong(m, c->ciphermask);
	for(i=0; i<c->nokauthsrv; i++)
		c->authmask |= 1<<c->okauthsrv[i]->id;
	putlong(m, c->authmask);

	sendmsg(m);
}

static mpint*
rpcdecrypt(AuthRpc *rpc, mpint *b)
{
	mpint *a;
	char *p;

	p = mptoa(b, 16, nil, 0);
	if(auth_rpc(rpc, "write", p, strlen(p)) != ARok)
		sysfatal("factotum rsa write: %r");
	free(p);
	if(auth_rpc(rpc, "read", nil, 0) != ARok)
		sysfatal("factotum rsa read: %r");
	a = strtomp(rpc->arg, nil, 16, nil);
	mpfree(b);
	return a;
}

static void
recv_ssh_cmsg_session_key(Conn *c, AuthRpc *rpc)
{
	int i, id, n, serverkeylen, hostkeylen;
	mpint *a, *b;
	uchar *buf;
	Msg *m;
	RSApriv *ksmall, *kbig;

	m = recvmsg(c, SSH_CMSG_SESSION_KEY);
	id = getbyte(m);
	c->cipher = nil;
	for(i=0; i<c->nokcipher; i++)
		if(c->okcipher[i]->id == id)
			c->cipher = c->okcipher[i];
	if(c->cipher == nil)
		sysfatal("invalid cipher selected");

	if(memcmp(getbytes(m, COOKIELEN), c->cookie, COOKIELEN) != 0)
		sysfatal("bad cookie");

	serverkeylen = mpsignif(c->serverkey->n);
	hostkeylen = mpsignif(c->hostkey->n);
	ksmall = kbig = nil;
	if(serverkeylen+128 <= hostkeylen){
		ksmall = c->serverpriv;
		kbig = nil;
	}else if(hostkeylen+128 <= serverkeylen){
		ksmall = nil;
		kbig = c->serverpriv;
	}else
		sysfatal("server session and host keys do not differ by at least 128 bits");

	b = getmpint(m);

	debug(DBG_CRYPTO, "encrypted with kbig is %B\n", b);
	if(kbig){
		a = rsadecrypt(kbig, b, nil);
		mpfree(b);
		b = a;
	}else
		b = rpcdecrypt(rpc, b);
	a = rsaunpad(b);
	mpfree(b);
	b = a;

	debug(DBG_CRYPTO, "encrypted with ksmall is %B\n", b);
	if(ksmall){
		a = rsadecrypt(ksmall, b, nil);
		mpfree(b);
		b = a;
	}else
		b = rpcdecrypt(rpc, b);
	a = rsaunpad(b);
	mpfree(b);
	b = a;

	debug(DBG_CRYPTO, "munged is %B\n", b);

	n = (mpsignif(b)+7)/8;
	if(n > SESSKEYLEN)
		sysfatal("client sent short session key");

	buf = emalloc(SESSKEYLEN);
	mptoberjust(b, buf, SESSKEYLEN);
	mpfree(b);

	for(i=0; i<SESSIDLEN; i++)
		buf[i] ^= c->sessid[i];

	memmove(c->sesskey, buf, SESSKEYLEN);

	debug(DBG_CRYPTO, "unmunged is %.*H\n", SESSKEYLEN, buf);

	c->flags = getlong(m);
	free(m);
}

static AuthInfo*
responselogin(char *user, char *resp)
{
	Chalstate *c;
	AuthInfo *ai;

	if((c = auth_challenge("proto=p9cr user=%q role=server", user)) == nil){
		sshlog("auth_challenge failed for %s", user);
		return nil;
	}
	c->resp = resp;
	c->nresp = strlen(resp);
	ai = auth_response(c);
	auth_freechal(c);
	return ai;
}

static AuthInfo*
authusername(Conn *c)
{
	char *p;
	AuthInfo *ai;

	/*
	 * hack for sam users: 'name numbers' gets tried as securid login.
	 */
	if(p = strchr(c->user, ' ')){
		*p++ = '\0';
		if((ai=responselogin(c->user, p)) != nil)
			return ai;
		*--p = ' ';
		sshlog("bad response: %s", c->user);
	}
	return nil;
}

static void
authsrvuser(Conn *c)
{
	int i;
	char *ns, *user;
	AuthInfo *ai;
	Msg *m;

	m = recvmsg(c, SSH_CMSG_USER);
	user = getstring(m);
	c->user = emalloc(strlen(user)+1);
	strcpy(c->user, user);
	free(m);

	ai = authusername(c);
	while(ai == nil){
		/*
		 * clumsy: if the client aborted the auth_tis early
		 * we don't send a new failure.  we check this by
		 * looking at c->unget, which is only used in that
		 * case.
		 */
		if(c->unget != nil)
			goto skipfailure;
		sendmsg(allocmsg(c, SSH_SMSG_FAILURE, 0));
	skipfailure:
		m = recvmsg(c, -1);
		for(i=0; i<c->nokauthsrv; i++)
			if(c->okauthsrv[i]->firstmsg == m->type){
				ai = (*c->okauthsrv[i]->fn)(c, m);
				break;
			}
		if(i==c->nokauthsrv)
			badmsg(m, 0);
	}
	sendmsg(allocmsg(c, SSH_SMSG_SUCCESS, 0));

	if(noworld(ai->cuid))
		ns = "/lib/namespace.noworld";
	else
		ns = nil;
	if(auth_chuid(ai, ns) < 0){
		sshlog("auth_chuid to %s: %r", ai->cuid);
		sysfatal("auth_chuid: %r");
	}
	sshlog("logged in as %s", ai->cuid);
	auth_freeAI(ai);
}

void
sshserverhandshake(Conn *c)
{
	char *p, buf[128];
	Biobuf *b;
	Attr *a;
	int i, afd;
	mpint *m;
	AuthRpc *rpc;
	RSApub *key;

	/*
	 * BUG: should use `attr' to get the key attributes
	 * after the read, but that's not implemented yet.
	 */
	if((b = Bopen("/mnt/factotum/ctl", OREAD)) == nil)
		sysfatal("open /mnt/factotum/ctl: %r");
	while((p = Brdline(b, '\n')) != nil){
		p[Blinelen(b)-1] = '\0';
		if(strstr(p, " proto=rsa ") && strstr(p, " service=sshserve "))
			break;
	}
	if(p == nil)
		sysfatal("no sshserve keys found in /mnt/factotum/ctl");
	a = _parseattr(p);
	Bterm(b);
	key = emalloc(sizeof(*key));
	if((p = _strfindattr(a, "n")) == nil)
		sysfatal("no n in sshserve key");
	if((key->n = strtomp(p, &p, 16, nil)) == nil || *p != 0)
		sysfatal("bad n in sshserve key");
	if((p = _strfindattr(a, "ek")) == nil)
		sysfatal("no ek in sshserve key");
	if((key->ek = strtomp(p, &p, 16, nil)) == nil || *p != 0)
		sysfatal("bad ek in sshserve key");
	_freeattr(a);

	if((afd = open("/mnt/factotum/rpc", ORDWR)) < 0)
		sysfatal("open /mnt/factotum/rpc: %r");
	if((rpc = auth_allocrpc(afd)) == nil)
		sysfatal("auth_allocrpc: %r");
	p = "proto=rsa role=client service=sshserve";
	if(auth_rpc(rpc, "start", p, strlen(p)) != ARok)
		sysfatal("auth_rpc start %s: %r", p);
	if(auth_rpc(rpc, "read", nil, 0) != ARok)
		sysfatal("auth_rpc read: %r");
	m = strtomp(rpc->arg, nil, 16, nil);
	if(mpcmp(m, key->n) != 0)
		sysfatal("key in /mnt/factotum/ctl does not match rpc key");
	mpfree(m);
	c->hostkey = key;

	/* send id string */
	fprint(c->fd[0], "SSH-1.5-Plan9\n");

	/* receive id string */
	if(readstrnl(c->fd[0], buf, sizeof buf) < 0)
		sysfatal("reading server version: %r");

	/* id string is "SSH-m.n-comment".  We need m=1, n>=5. */
	if(strncmp(buf, "SSH-", 4) != 0
	|| strtol(buf+4, &p, 10) != 1
	|| *p != '.'
	|| strtol(p+1, &p, 10) < 5
	|| *p != '-')
		sysfatal("protocol mismatch; got %s, need SSH-1.x for x>=5", buf);

	for(i=0; i<COOKIELEN; i++)
		c->cookie[i] = fastrand();
	calcsessid(c);
	send_ssh_smsg_public_key(c);
	recv_ssh_cmsg_session_key(c, rpc);
	auth_freerpc(rpc);
	close(afd);

	c->cstate = (*c->cipher->init)(c, 1);		/* turns on encryption */
	sendmsg(allocmsg(c, SSH_SMSG_SUCCESS, 0));

	authsrvuser(c);
}
