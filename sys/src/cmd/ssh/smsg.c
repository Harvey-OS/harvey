#include "ssh.h"

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

static void
recv_ssh_cmsg_session_key(Conn *c)
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
		kbig = c->hostpriv;
	}else if(hostkeylen+128 <= serverkeylen){
		ksmall = c->hostpriv;
		kbig = c->serverpriv;
	}else
		sysfatal("server session and host keys do not differ by at least 128 bits");

	b = getmpint(m);

	debug(DBG_CRYPTO, "encrypted with kbig is %B\n", b);
	a = rsadecrypt(kbig, b, nil);
	mpfree(b);
	b = a;
	a = rsaunpad(b);
	mpfree(b);
	b = a;

	debug(DBG_CRYPTO, "encrypted with ksmall is %B\n", b);
	a = rsadecrypt(ksmall, b, nil);
	mpfree(b);
	b = a;
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
		sendmsg(allocmsg(c, SSH_SMSG_FAILURE, 0));
		m = recvmsg(c, 0);
		if(m == nil)
			badmsg(m, 0);
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
	char buf[128], *p;
	int i;

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
	recv_ssh_cmsg_session_key(c);

	c->cstate = (*c->cipher->init)(c, 1);		/* turns on encryption */
	sendmsg(allocmsg(c, SSH_SMSG_SUCCESS, 0));

	authsrvuser(c);
}
