#include "ssh.h"
#include <bio.h>

typedef struct Key Key;
struct Key
{
	mpint *mod;
	mpint *ek;
	char *comment;
};

typedef struct Achan Achan;
struct Achan
{
	int open;
	u32int chan;	/* of remote */
	uchar lbuf[4];
	uint nlbuf;
	uint len;
	uchar *data;
	int ndata;
	int needeof;
	int needclosed;
};

Achan achan[16];

static char*
find(char **f, int nf, char *k)
{
	int i, len;

	len = strlen(k);
	for(i=1; i<nf; i++)	/* i=1: f[0] is "key" */
		if(strncmp(f[i], k, len) == 0 && f[i][len] == '=')
			return f[i]+len+1;
	return nil;
}

static int
listkeys(Key **kp)
{
	Biobuf *b;
	Key *k;
	int nk;
	char *p, *f[20];
	int nf;
	mpint *mod, *ek;
	
	*kp = nil;
	if((b = Bopen("/mnt/factotum/ctl", OREAD)) == nil)
		return -1;
	
	k = nil;
	nk = 0;
	while((p = Brdline(b, '\n')) != nil){
		p[Blinelen(b)-1] = '\0';
		nf = tokenize(p, f, nelem(f));
		if(nf == 0 || strcmp(f[0], "key") != 0)
			continue;
		p = find(f, nf, "proto");
		if(p == nil || strcmp(p, "rsa") != 0)
			continue;
		p = find(f, nf, "n");
		if(p == nil || (mod = strtomp(p, nil, 16, nil)) == nil)
			continue;
		p = find(f, nf, "ek");
		if(p == nil || (ek = strtomp(p, nil, 16, nil)) == nil){
			mpfree(mod);
			continue;
		}
		p = find(f, nf, "comment");
		if(p == nil)
			p = "";
		k = erealloc(k, (nk+1)*sizeof(k[0]));
		k[nk].mod = mod;
		k[nk].ek = ek;
		k[nk].comment = emalloc(strlen(p)+1);
		strcpy(k[nk].comment, p);
		nk++;
	}
	Bterm(b);
	*kp = k;
	return nk;	
}


static int
dorsa(mpint *mod, mpint *exp, mpint *chal, uchar chalbuf[32])
{
	int afd;
	AuthRpc *rpc;
	mpint *m;
	char buf[4096], *p;
	mpint *decr, *unpad;

	USED(exp);

	snprint(buf, sizeof buf, "proto=rsa service=ssh role=client");
	if((afd = open("/mnt/factotum/rpc", ORDWR)) < 0){
		debug(DBG_AUTH, "open /mnt/factotum/rpc: %r\n");
		return -1;
	}
	if((rpc = auth_allocrpc(afd)) == nil){
		debug(DBG_AUTH, "auth_allocrpc: %r\n");
		close(afd);
		return -1;
	}
	if(auth_rpc(rpc, "start", buf, strlen(buf)) != ARok){
		debug(DBG_AUTH, "auth_rpc start failed: %r\n");
	Die:
		auth_freerpc(rpc);
		close(afd);
		return -1;
	}
	m = nil;
	debug(DBG_AUTH, "trying factotum rsa keys\n");
	while(auth_rpc(rpc, "read", nil, 0) == ARok){
		debug(DBG_AUTH, "try %s\n", (char*)rpc->arg);
		m = strtomp(rpc->arg, nil, 16, nil);
		if(mpcmp(m, mod) == 0)
			break;
		mpfree(m);
		m = nil;
	}
	if(m == nil)
		goto Die;
	mpfree(m);
	
	p = mptoa(chal, 16, nil, 0);
	if(p == nil){
		debug(DBG_AUTH, "\tmptoa failed: %r\n");
		goto Die;
	}
	if(auth_rpc(rpc, "write", p, strlen(p)) != ARok){
		debug(DBG_AUTH, "\tauth_rpc write failed: %r\n");
		free(p);
		goto Die;
	}
	free(p);
	if(auth_rpc(rpc, "read", nil, 0) != ARok){
		debug(DBG_AUTH, "\tauth_rpc read failed: %r\n");
		goto Die;
	}
	decr = strtomp(rpc->arg, nil, 16, nil);
	if(decr == nil){
		debug(DBG_AUTH, "\tdecr %s failed\n", rpc->arg);
		goto Die;
	}
	debug(DBG_AUTH, "\tdecrypted %B\n", decr);
	unpad = rsaunpad(decr);
	if(unpad == nil){
		debug(DBG_AUTH, "\tunpad %B failed\n", decr);
		mpfree(decr);
		goto Die;
	}
	debug(DBG_AUTH, "\tunpadded %B\n", unpad);
	mpfree(decr);
	mptoberjust(unpad, chalbuf, 32);
	mpfree(unpad);
	auth_freerpc(rpc);
	close(afd);
	return 0;
}

int
startagent(Conn *c)
{
	int ret;
	Msg *m;

	m = allocmsg(c, SSH_CMSG_AGENT_REQUEST_FORWARDING, 0);
	sendmsg(m);

	m = recvmsg(c, -1);
	switch(m->type){
	case SSH_SMSG_SUCCESS:
		debug(DBG_AUTH, "agent allocated\n");
		ret = 0;
		break;
	case SSH_SMSG_FAILURE:
		debug(DBG_AUTH, "agent failed to allocate\n");
		ret = -1;
		break;
	default:
		badmsg(m, 0);
		ret = -1;
		break;
	}
	free(m);
	return ret;
}

void handlefullmsg(Conn*, Achan*);

void
handleagentmsg(Msg *m)
{
	u32int chan, len;
	int n;
	Achan *a;

	assert(m->type == SSH_MSG_CHANNEL_DATA);

	debug(DBG_AUTH, "agent data\n");
	debug(DBG_AUTH, "\t%.*H\n", (int)(m->ep - m->rp), m->rp);
	chan = getlong(m);
	len = getlong(m);
	if(m->rp+len != m->ep)
		sysfatal("got bad channel data");

	if(chan >= nelem(achan))
		error("bad channel in agent request");

	a = &achan[chan];

	while(m->rp < m->ep){
		if(a->nlbuf < 4){
			a->lbuf[a->nlbuf++] = getbyte(m);
			if(a->nlbuf == 4){
				a->len = (a->lbuf[0]<<24) | (a->lbuf[1]<<16) | (a->lbuf[2]<<8) | a->lbuf[3];
				a->data = erealloc(a->data, a->len);
				a->ndata = 0;
			}
			continue;
		}
		if(a->ndata < a->len){
			n = a->len - a->ndata;
			if(n > m->ep - m->rp)
				n = m->ep - m->rp;
			memmove(a->data+a->ndata, getbytes(m, n), n);
			a->ndata += n;
		}
		if(a->ndata == a->len){
			handlefullmsg(m->c, a);
			a->nlbuf = 0;
		}
	}
}

void
handlefullmsg(Conn *c, Achan *a)
{
	int i;
	u32int chan, len, n, rt;
	uchar type;
	Msg *m, mm;
	Msg *r;
	Key *k;
	int nk;
	mpint *mod, *ek, *chal;
	uchar sessid[16];
	uchar chalbuf[32];
	uchar digest[16];
	DigestState *s;
	static int first;

	assert(a->len == a->ndata);

	chan = a->chan;
	mm.rp = a->data;
	mm.ep = a->data+a->ndata;
	mm.c = c;
	m = &mm;

	type = getbyte(m);

	if(first == 0){
		first++;
		fmtinstall('H', encodefmt);
	}

	switch(type){
	default:
		debug(DBG_AUTH, "unknown msg type\n");
	Failure:
		debug(DBG_AUTH, "agent sending failure\n");
		r = allocmsg(m->c, SSH_MSG_CHANNEL_DATA, 13);
		putlong(r, chan);
		putlong(r, 5);
		putlong(r, 1);
		putbyte(r, SSH_AGENT_FAILURE);
		sendmsg(r);
		return;

	case SSH_AGENTC_REQUEST_RSA_IDENTITIES:
		debug(DBG_AUTH, "agent request identities\n");
		nk = listkeys(&k);
		if(nk < 0)
			goto Failure;
		len = 1+4;	/* type, nk */
		for(i=0; i<nk; i++){
			len += 4;
			len += 2+(mpsignif(k[i].ek)+7)/8;
			len += 2+(mpsignif(k[i].mod)+7)/8;
			len += 4+strlen(k[i].comment);
		}
		r = allocmsg(m->c, SSH_MSG_CHANNEL_DATA, 12+len);
		putlong(r, chan);
		putlong(r, len+4);
		putlong(r, len);
		putbyte(r, SSH_AGENT_RSA_IDENTITIES_ANSWER);
		putlong(r, nk);
		for(i=0; i<nk; i++){
			debug(DBG_AUTH, "\t%B %B %s\n", k[i].ek, k[i].mod, k[i].comment);
			putlong(r, mpsignif(k[i].mod));
			putmpint(r, k[i].ek);
			putmpint(r, k[i].mod);
			putstring(r, k[i].comment);
			mpfree(k[i].ek);
			mpfree(k[i].mod);
			free(k[i].comment);
		}
		free(k);
		sendmsg(r);
		break;

	case SSH_AGENTC_RSA_CHALLENGE:
		n = getlong(m);
		USED(n);	/* number of bits in key; who cares? */
		ek = getmpint(m);
		mod = getmpint(m);
		chal = getmpint(m);
		memmove(sessid, getbytes(m, 16), 16);
		rt = getlong(m);
		debug(DBG_AUTH, "agent challenge %B %B %B %ud (%p %p)\n",
			ek, mod, chal, rt, m->rp, m->ep);
		if(rt != 1 || dorsa(mod, ek, chal, chalbuf) < 0){
			mpfree(ek);
			mpfree(mod);
			mpfree(chal);
			goto Failure;
		}
		s = md5(chalbuf, 32, nil, nil);
		md5(sessid, 16, digest, s);
		r = allocmsg(m->c, SSH_MSG_CHANNEL_DATA, 12+1+16);
		putlong(r, chan);
		putlong(r, 4+16+1);
		putlong(r, 16+1);
		putbyte(r, SSH_AGENT_RSA_RESPONSE);
		putbytes(r, digest, 16);
		debug(DBG_AUTH, "digest %.16H\n", digest);
		sendmsg(r);
		mpfree(ek);
		mpfree(mod);
		mpfree(chal);
		return;

	case SSH_AGENTC_ADD_RSA_IDENTITY:
		goto Failure;
/*
		n = getlong(m);
		pubmod = getmpint(m);
		pubexp = getmpint(m);
		privexp = getmpint(m);
		pinversemodq = getmpint(m);
		p = getmpint(m);
		q = getmpint(m);
		comment = getstring(m);
		add to factotum;
		send SSH_AGENT_SUCCESS or SSH_AGENT_FAILURE;
*/

	case SSH_AGENTC_REMOVE_RSA_IDENTITY:
		goto Failure;
/*
		n = getlong(m);
		pubmod = getmpint(m);
		pubexp = getmpint(m);
		tell factotum to del key
		send SSH_AGENT_SUCCESS or SSH_AGENT_FAILURE;
*/
	}
}

void
handleagentopen(Msg *m)
{
	int i;
	u32int remote;

	assert(m->type == SSH_SMSG_AGENT_OPEN);
	remote = getlong(m);
	debug(DBG_AUTH, "agent open %d\n", remote);

	for(i=0; i<nelem(achan); i++)
		if(achan[i].open == 0 && achan[i].needeof == 0 && achan[i].needclosed == 0)
			break;
	if(i == nelem(achan)){
		m = allocmsg(m->c, SSH_MSG_CHANNEL_OPEN_FAILURE, 4);
		putlong(m, remote);
		sendmsg(m);
		return;
	}

	debug(DBG_AUTH, "\tremote %d is local %d\n", remote, i);
	achan[i].open = 1;
	achan[i].needeof = 1;
	achan[i].needclosed = 1;
	achan[i].nlbuf = 0;
	achan[i].chan = remote;
	m = allocmsg(m->c, SSH_MSG_CHANNEL_OPEN_CONFIRMATION, 8);
	putlong(m, remote);
	putlong(m, i);
	sendmsg(m);
}

void
handleagentieof(Msg *m)
{
	u32int local;

	assert(m->type == SSH_MSG_CHANNEL_INPUT_EOF);
	local = getlong(m);
	debug(DBG_AUTH, "agent close %d\n", local);
	if(local < nelem(achan)){
		debug(DBG_AUTH, "\tlocal %d is remote %d\n", local, achan[local].chan);
		achan[local].open = 0;
/*
		m = allocmsg(m->c, SSH_MSG_CHANNEL_OUTPUT_CLOSED, 4);
		putlong(m, achan[local].chan);
		sendmsg(m);
*/
		if(achan[local].needeof){
			achan[local].needeof = 0;
			m = allocmsg(m->c, SSH_MSG_CHANNEL_INPUT_EOF, 4);
			putlong(m, achan[local].chan);
			sendmsg(m);
		}
	}
}

void
handleagentoclose(Msg *m)
{
	u32int local;

	assert(m->type == SSH_MSG_CHANNEL_OUTPUT_CLOSED);
	local = getlong(m);
	debug(DBG_AUTH, "agent close %d\n", local);
	if(local < nelem(achan)){
		debug(DBG_AUTH, "\tlocal %d is remote %d\n", local, achan[local].chan);
		if(achan[local].needclosed){
			achan[local].needclosed = 0;
			m = allocmsg(m->c, SSH_MSG_CHANNEL_OUTPUT_CLOSED, 4);
			putlong(m, achan[local].chan);
			sendmsg(m);
		}
	}
}
