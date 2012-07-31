#include <u.h>
#include <libc.h>
#include <mp.h>
#include <libsec.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <auth.h>
#include <ip.h>
#include <pool.h>
#include "netssh.h"

#undef VERIFYKEYS		/* TODO until it's fixed */

enum {
	Errnokey = -2,	/* no key on keyring */
	Errnoverify,	/* factotum found a key, but verification failed */
	Errfactotum,	/* factotum failure (e.g., no key) */
	Errnone,	/* key verified */
};

static int dh_server(Conn *, Packet *, mpint *, int);
static void genkeys(Conn *, uchar [], mpint *);

/*
 * Second Oakley Group from RFC 2409
 */
static char *group1p =
         "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
         "29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
         "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
         "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
         "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE65381"
         "FFFFFFFFFFFFFFFF";

/*
 * 2048-bit MODP group (id 14) from RFC 3526
*/
static char *group14p =
      "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
      "29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
      "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
      "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
      "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D"
      "C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F"
      "83655D23DCA3AD961C62F356208552BB9ED529077096966D"
      "670C354E4ABC9804F1746C08CA18217C32905E462E36CE3B"
      "E39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9"
      "DE2BCBF6955817183995497CEA956AE515D2261898FA0510"
      "15728E5A8AACAA68FFFFFFFFFFFFFFFF";

mpint *two, *p1, *p14;
int nokeyverify;

static DSApriv mydsskey;
static RSApriv myrsakey;

void
dh_init(PKA *pkas[])
{
	char *buf, *p, *st, *end;
	int fd, n, k;

	if(debug > 1)
		sshdebug(nil, "dh_init");
	k = 0;
	pkas[k] = nil;
	fmtinstall('M', mpfmt);
	two = strtomp("2", nil, 10, nil);
	p1 = strtomp(group1p, nil, 16, nil);
	p14 = strtomp(group14p, nil, 16, nil);

	/*
	 * this really should be done through factotum
	 */
	p = getenv("rsakey");
	if (p != nil) {
		remove("/env/rsakey");
		st = buf = p;
		end = buf + strlen(p);
	} else {
		/*
		 * it would be better to use bio and rdline here instead of
		 * reading all of factotum's contents into memory at once.
		 */
		buf = emalloc9p(Maxfactotum);
		fd = open("rsakey", OREAD);
		if (fd < 0 && (fd = open("/mnt/factotum/ctl", OREAD)) < 0)
			goto norsa;
		n = readn(fd, buf, Maxfactotum - 1);
		buf[n >= 0? n: 0] = 0;
		close(fd);
		assert(n < Maxfactotum - 1);

		st = strstr(buf, "proto=rsa");
		if (st == nil) {
			sshlog(nil, "no proto=rsa key in factotum");
			goto norsa;
		}
		end = st;
		for (; st > buf && *st != '\n'; --st)
			;
		for (; end < buf + Maxfactotum && *end != '\n'; ++end)
			;
	}
	p = strstr(st, " n=");
	if (p == nil || p > end) {
		sshlog(nil, "no key (n) found");
		free(buf);
		return;
	}
	myrsakey.pub.n = strtomp(p+3, nil, 16, nil);
	if (debug > 1)
		sshdebug(nil, "n=%M", myrsakey.pub.n);
	p = strstr(st, " ek=");
	if (p == nil || p > end) {
		sshlog(nil, "no key (ek) found");
		free(buf);
		return;
	}
	pkas[k++] = &rsa_pka;
	pkas[k] = nil;
	myrsakey.pub.ek = strtomp(p+4, nil, 16, nil);
	if (debug > 1)
		sshdebug(nil, "ek=%M", myrsakey.pub.ek);
	p = strstr(st, " !dk=");
	if (p == nil) {
		p = strstr(st, "!dk?");
		if (p == nil || p > end) {
			// sshlog(nil, "no key (dk) found");
			free(buf);
			return;
		}
		goto norsa;
	}
	myrsakey.dk = strtomp(p+5, nil, 16, nil);
	if (debug > 1)
		sshdebug(nil, "dk=%M", myrsakey.dk);
norsa:
	free(buf);

	p = getenv("dsskey");
	if (p != nil) {
		remove("/env/dsskey");
		buf = p;
		end = buf + strlen(p);
	} else {
		/*
		 * it would be better to use bio and rdline here instead of
		 * reading all of factotum's contents into memory at once.
		 */
		buf = emalloc9p(Maxfactotum);
		fd = open("dsskey", OREAD);
		if (fd < 0 && (fd = open("/mnt/factotum/ctl", OREAD)) < 0)
			return;
		n = readn(fd, buf, Maxfactotum - 1);
		buf[n >= 0? n: 0] = 0;
		close(fd);
		assert(n < Maxfactotum - 1);

		st = strstr(buf, "proto=dsa");
		if (st == nil) {
			sshlog(nil, "no proto=dsa key in factotum");
			free(buf);
			return;
		}
		end = st;
		for (; st > buf && *st != '\n'; --st)
			;
		for (; end < buf + Maxfactotum && *end != '\n'; ++end)
			;
	}
	p = strstr(buf, " p=");
	if (p == nil || p > end) {
		sshlog(nil, "no key (p) found");
		free(buf);
		return;
	}
	mydsskey.pub.p = strtomp(p+3, nil, 16, nil);
	p = strstr(buf, " q=");
	if (p == nil || p > end) {
		sshlog(nil, "no key (q) found");
		free(buf);
		return;
	}
	mydsskey.pub.q = strtomp(p+3, nil, 16, nil);
	p = strstr(buf, " alpha=");
	if (p == nil || p > end) {
		sshlog(nil, "no key (g) found");
		free(buf);
		return;
	}
	mydsskey.pub.alpha = strtomp(p+7, nil, 16, nil);
	p = strstr(buf, " key=");
	if (p == nil || p > end) {
		sshlog(nil, "no key (y) found");
		free(buf);
		return;
	}
	mydsskey.pub.key = strtomp(p+5, nil, 16, nil);
	pkas[k++] = &dss_pka;
	pkas[k] = nil;
	p = strstr(buf, " !secret=");
	if (p == nil) {
		p = strstr(buf, "!secret?");
		if (p == nil || p > end)
			sshlog(nil, "no key (x) found");
		free(buf);
		return;
	}
	mydsskey.secret = strtomp(p+9, nil, 16, nil);
	free(buf);
}

static Packet *
rsa_ks(Conn *c)
{
	Packet *ks;

	if (myrsakey.pub.ek == nil || myrsakey.pub.n == nil) {
		sshlog(c, "no public RSA key info");
		return nil;
	}
	ks = new_packet(c);
	add_string(ks, "ssh-rsa");
	add_mp(ks, myrsakey.pub.ek);
	add_mp(ks, myrsakey.pub.n);
	return ks;
}

static void
esma_encode(uchar *h, uchar *em, int nb)
{
	int n, i;
	uchar hh[SHA1dlen];
	static uchar sha1der[] = {
		0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e,
		0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14,
	};

	sha1(h, SHA1dlen, hh, nil);
	n = nb - (nelem(sha1der) + SHA1dlen) - 3;
	i = 0;
	em[i++] = 0;
	em[i++] = 1;
	memset(em + i, 0xff, n);
	i += n;
	em[i++] = 0;
	memmove(em + i, sha1der, sizeof sha1der);
	i += sizeof sha1der;
	memmove(em + i, hh, SHA1dlen);
}

static Packet *
rsa_sign(Conn *c, uchar *m, int nm)
{
	AuthRpc *ar;
	Packet *sig;
	mpint *s, *mm;
	int fd, n, nbit;
	uchar hh[SHA1dlen];
	uchar *sstr, *em;

	if (myrsakey.dk) {
		nbit = mpsignif (myrsakey.pub.n);
		n = (nbit + 7) / 8;
		sstr = emalloc9p(n);
		em = emalloc9p(n);
		/* Compute s: RFC 3447 */
		esma_encode(m, em, n);
		mm = betomp(em, n, nil);
		s = mpnew(nbit);
		mpexp(mm, myrsakey.dk, myrsakey.pub.n, s);
		mptobe(s, sstr, n, nil);
		mpfree(mm);
		mpfree(s);
		free(em);
	} else {
		fd = open("/mnt/factotum/rpc", ORDWR);
		if (fd < 0)
			return nil;
		sha1(m, nm, hh, nil);
		ar = auth_allocrpc(fd);
		if (ar == nil ||
		    auth_rpc(ar, "start", "role=sign proto=rsa", 19) != ARok ||
		    auth_rpc(ar, "write", hh, SHA1dlen) != ARok ||
		    auth_rpc(ar, "read", nil, 0) != ARok ||
		    ar->arg == nil) {
			sshdebug(c, "got error in factotum: %r");
			auth_freerpc(ar);
			close(fd);
			return nil;
		}
		sstr = emalloc9p(ar->narg);
		memmove(sstr, ar->arg, ar->narg);
		n = ar->narg;

		auth_freerpc(ar);
		close(fd);
	}
	sig = new_packet(c);
	add_string(sig, pkas[c->pkalg]->name);
	add_block(sig, sstr, n);
	free(sstr);
	return sig;
}

/*
 * 0 - If factotum failed, e.g. no key
 * 1 - If key is verified
 * -1 - If factotum found a key, but the verification fails
 */
static int
rsa_verify(Conn *c, uchar *m, int nm, char *user, char *sig, int)
{
	int fd, n, retval, nbit;
	char *buf, *p, *sigblob;
	uchar *sstr, *em;
	uchar hh[SHA1dlen];
	mpint *s, *mm, *rsa_exponent, *host_modulus;
	AuthRpc *ar;

	sshdebug(c, "in rsa_verify for connection: %d", c->id);
	SET(rsa_exponent, host_modulus);
	USED(rsa_exponent, host_modulus);
	if (0 && rsa_exponent) {
		nbit = mpsignif(host_modulus);
		n = (nbit + 7) / 8;
		em = emalloc9p(n);
		/* Compute s: RFC 3447 */
		esma_encode(m, em, n);
		mm = betomp(em, n, nil);
		s = mpnew(1024);
		mpexp(mm, rsa_exponent, host_modulus, s);
		sstr = emalloc9p(n);
		mptobe(s, sstr, n, nil);
		free(em);
		mpfree(mm);
		mpfree(s);
		retval = memcmp(sig, sstr, n);
		free(sstr);
		retval = (retval == 0);
	} else {
		retval = 1;
		fd = open("/mnt/factotum/rpc", ORDWR);
		if (fd < 0) {
			sshdebug(c, "could not open factotum RPC: %r");
			return 0;
		}
		buf = emalloc9p(Blobsz / 2);
		sigblob = emalloc9p(Blobsz);
		p = (char *)get_string(nil, (uchar *)sig, buf, Blobsz / 2, nil);
		get_string(nil, (uchar *)p, sigblob, Blobsz, &n);
		sha1(m, nm, hh, nil);
		if (user != nil)
			p = smprint("role=verify proto=rsa user=%s", user);
		else
			p = smprint("role=verify proto=rsa sys=%s", c->remote);

		ar = auth_allocrpc(fd);
		if (ar == nil || auth_rpc(ar, "start", p, strlen(p)) != ARok ||
		    auth_rpc(ar, "write", hh, SHA1dlen) != ARok ||
		    auth_rpc(ar, "write", sigblob, n) != ARok ||
		    auth_rpc(ar, "read", nil, 0) != ARok) {
			sshdebug(c, "got error in factotum: %r");
			retval = 0;
		} else {
			sshdebug(c, "factotum returned %s", ar->ibuf);
			if (strstr(ar->ibuf, "does not verify") != nil)
				retval = -1;
		}
		if (ar != nil)
			auth_freerpc(ar);
		free(p);
		close(fd);
		free(sigblob);
		free(buf);
	}
	return retval;
}

static Packet *
dss_ks(Conn *c)
{
	Packet *ks;

	if (mydsskey.pub.p == nil)
		return nil;
	ks = new_packet(c);
	add_string(ks, "ssh-dss");
	add_mp(ks, mydsskey.pub.p);
	add_mp(ks, mydsskey.pub.q);
	add_mp(ks, mydsskey.pub.alpha);
	add_mp(ks, mydsskey.pub.key);
	return ks;
}

static Packet *
dss_sign(Conn *c, uchar *m, int nm)
{
	AuthRpc *ar;
	DSAsig *s;
	Packet *sig;
	mpint *mm;
	int fd;
	uchar sstr[2*SHA1dlen];

	sha1(m, nm, sstr, nil);
	sig = new_packet(c);
	add_string(sig, pkas[c->pkalg]->name);
	if (mydsskey.secret) {
		mm = betomp(sstr, SHA1dlen, nil);
		s = dsasign(&mydsskey, mm);
		mptobe(s->r, sstr, SHA1dlen, nil);
		mptobe(s->s, sstr+SHA1dlen, SHA1dlen, nil);
		dsasigfree(s);
		mpfree(mm);
	} else {
		fd = open("/mnt/factotum/rpc", ORDWR);
		if (fd < 0)
			return nil;
		ar = auth_allocrpc(fd);
		if (ar == nil ||
		    auth_rpc(ar, "start", "role=sign proto=dsa", 19) != ARok ||
		    auth_rpc(ar, "write", sstr, SHA1dlen) != ARok ||
		    auth_rpc(ar, "read", nil, 0) != ARok) {
			sshdebug(c, "got error in factotum: %r");
			auth_freerpc(ar);
			close(fd);
			return nil;
		}
		memmove(sstr, ar->arg, ar->narg);
		auth_freerpc(ar);
		close(fd);
	}
	add_block(sig, sstr, 2*SHA1dlen);
	return sig;
}

static int
dss_verify(Conn *c, uchar *m, int nm, char *user, char *sig, int nsig)
{
	sshdebug(c, "in dss_verify for connection: %d", c->id);
	USED(m, nm, user, sig, nsig);
	return 0;
}

static int
dh_server1(Conn *c, Packet *pack1)
{
	return dh_server(c, pack1, p1, 1024);
}

static int
dh_server14(Conn *c, Packet *pack1)
{
	return dh_server(c, pack1, p14, 2048);
}

static int
dh_server(Conn *c, Packet *pack1, mpint *grp, int nbit)
{
	Packet *pack2, *ks, *sig;
	mpint *y, *e, *f, *k;
	int n, ret;
	uchar h[SHA1dlen];

	ret = -1;
	qlock(&c->l);
	f = mpnew(nbit);
	k = mpnew(nbit);

	/* Compute f: RFC4253 */
	y = mprand(nbit / 8, genrandom, nil);
	if (debug > 1)
		sshdebug(c, "y=%M", y);
	mpexp(two, y, grp, f);
	if (debug > 1)
		sshdebug(c, "f=%M", f);

	/* Compute k: RFC4253 */
	if (debug > 1)
		dump_packet(pack1);
	e = get_mp(pack1->payload+1);
	if (debug > 1)
		sshdebug(c, "e=%M", e);
	mpexp(e, y, grp, k);
	if (debug > 1)
		sshdebug(c, "k=%M", k);

	/* Compute H: RFC 4253 */
	pack2 = new_packet(c);
	sshdebug(c, "ID strings: %s---%s", c->otherid, MYID);
	add_string(pack2, c->otherid);
	add_string(pack2, MYID);
	if (debug > 1) {
		fprint(2, "received kexinit:");
		dump_packet(c->rkexinit);
		fprint(2, "\nsent kexinit:");
		dump_packet(c->skexinit);
	}
	add_block(pack2, c->rkexinit->payload, c->rkexinit->rlength - 1);
	add_block(pack2, c->skexinit->payload,
		c->skexinit->rlength - c->skexinit->pad_len - 1);
	sig = nil;
	ks = pkas[c->pkalg]->ks(c);
	if (ks == nil)
		goto err;
	add_block(pack2, ks->payload, ks->rlength - 1);
	add_mp(pack2, e);
	add_mp(pack2, f);
	add_mp(pack2, k);
	sha1(pack2->payload, pack2->rlength - 1, h, nil);

	if (c->got_sessid == 0) {
		memmove(c->sessid, h, SHA1dlen);
		c->got_sessid = 1;
	}
	sig = pkas[c->pkalg]->sign(c, h, SHA1dlen);
	if (sig == nil) {
		sshlog(c, "failed to generate signature: %r");
		goto err;
	}

	/* Send (K_s || f || s) to client: RFC4253 */
	init_packet(pack2);
	pack2->c = c;
	add_byte(pack2, SSH_MSG_KEXDH_REPLY);
	add_block(pack2, ks->payload, ks->rlength - 1);
	add_mp(pack2, f);
	add_block(pack2, sig->payload, sig->rlength - 1);
	if (debug > 1)
		dump_packet(pack2);
	n = finish_packet(pack2);
	if (debug > 1) {
		sshdebug(c, "writing %d bytes: len %d", n, nhgetl(pack2->nlength));
		dump_packet(pack2);
	}
	iowrite(c->dio, c->datafd, pack2->nlength, n);

	genkeys(c, h, k);

	/* Send SSH_MSG_NEWKEYS */
	init_packet(pack2);
	pack2->c = c;
	add_byte(pack2, SSH_MSG_NEWKEYS);
	n = finish_packet(pack2);
	iowrite(c->dio, c->datafd, pack2->nlength, n);
	ret = 0;
err:
	mpfree(f);
	mpfree(e);
	mpfree(k);
	mpfree(y);
	free(sig);
	free(ks);
	free(pack2);
	qunlock(&c->l);
	return ret;
}

static int
dh_client11(Conn *c, Packet *)
{
	Packet *p;
	int n;

	if (c->e)
		mpfree(c->e);
	c->e = mpnew(1024);

	/* Compute e: RFC4253 */
	if (c->x)
		mpfree(c->x);
	c->x = mprand(128, genrandom, nil);
	mpexp(two, c->x, p1, c->e);

	p = new_packet(c);
	add_byte(p, SSH_MSG_KEXDH_INIT);
	add_mp(p, c->e);
	n = finish_packet(p);
	iowrite(c->dio, c->datafd, p->nlength, n);
	free(p);
	return 0;
}

static int
findkeyinuserring(Conn *c, RSApub *srvkey)
{
	int n;
	char *home, *newkey, *r;

	home = getenv("home");
	if (home == nil) {
		newkey = "No home directory for key file";
		free(keymbox.msg);
		keymbox.msg = smprint("b%04ld%s", strlen(newkey), newkey);
		return -1;
	}

	r = smprint("%s/lib/keyring", home);
	free(home);
	if ((n = findkey(r, c->remote, srvkey)) != KeyOk) {
		newkey = smprint("ek=%M n=%M", srvkey->ek, srvkey->n);
		free(keymbox.msg);
		keymbox.msg = smprint("%c%04ld%s", n == NoKeyFile || n == NoKey?
			'c': 'b', strlen(newkey), newkey);
		free(newkey);

		nbsendul(keymbox.mchan, 1);
		recvul(keymbox.mchan);
		if (keymbox.msg == nil || keymbox.msg[0] == 'n') {
			free(keymbox.msg);
			keymbox.msg = nil;
			newkey = "Server key reject";
			keymbox.msg = smprint("f%04ld%s", strlen(newkey), newkey);
			return -1;
		}
		sshdebug(c, "adding key");
		if (keymbox.msg[0] == 'y')
			appendkey(r, c->remote, srvkey);
		else if (keymbox.msg[0] == 'r')
			replacekey(r, c->remote, srvkey);
	}
	free(r);
	return 0;
}

static int
verifyhostkey(Conn *c, RSApub *srvkey, Packet *sig)
{
	int fd, n;
	char *newkey;
	uchar h[SHA1dlen];

	sshdebug(c, "verifying server signature");
	if (findkey("/sys/lib/ssh/keyring", c->remote, srvkey) != KeyOk &&
	    findkeyinuserring(c, srvkey) < 0) {
		nbsendul(keymbox.mchan, 1);
		mpfree(srvkey->ek);
		mpfree(srvkey->n);
		return Errnokey;
	}

	newkey = smprint("key proto=rsa role=verify sys=%s size=%d ek=%M n=%M",
		c->remote, mpsignif(srvkey->n), srvkey->ek, srvkey->n);
	if (newkey == nil) {
		sshlog(c, "out of memory");
		threadexits("out of memory");
	}

	fd = open("/mnt/factotum/ctl", OWRITE);
	if (fd >= 0)
		write(fd, newkey, strlen(newkey));
		/* leave fd open */
	else
		sshdebug(c, "factotum open failed: %r");

	free(newkey);
	mpfree(srvkey->ek);
	mpfree(srvkey->n);
	free(keymbox.msg);
	keymbox.msg = nil;

	n = pkas[c->pkalg]->verify(c, h, SHA1dlen, nil, (char *)sig->payload,
		sig->rlength);

	/* fd is perhaps still open */
	if (fd >= 0) {
		/* sys here is a dotted-quad ip address */
		newkey = smprint("delkey proto=rsa role=verify sys=%s",
			c->remote);
		if (newkey) {
			seek(fd, 0, 0);
			write(fd, newkey, strlen(newkey));
			free(newkey);
		}
		close(fd);
	}
	return n;
}

static int
dh_client12(Conn *c, Packet *p)
{
	int n, retval;
#ifdef VERIFYKEYS
	char *newkey;
#endif
	char buf[10];
	uchar *q;
	uchar h[SHA1dlen];
	mpint *f, *k;
	Packet *ks, *sig, *pack2;
	RSApub *srvkey;

	ks = new_packet(c);
	sig = new_packet(c);
	pack2 = new_packet(c);

	q = get_string(p, p->payload+1, (char *)ks->payload, Maxpktpay, &n);
	ks->rlength = n + 1;
	f = get_mp(q);
	q += nhgetl(q) + 4;
	get_string(p, q, (char *)sig->payload, Maxpktpay, &n);
	sig->rlength = n;
	k = mpnew(1024);
	mpexp(f, c->x, p1, k);

	/* Compute H: RFC 4253 */
	init_packet(pack2);
	pack2->c = c;
	if (debug > 1)
		sshdebug(c, "ID strings: %s---%s", c->otherid, MYID);
	add_string(pack2, MYID);
	add_string(pack2, c->otherid);
	if (debug > 1) {
		fprint(2, "received kexinit:");
		dump_packet(c->rkexinit);
		fprint(2, "\nsent kexinit:");
		dump_packet(c->skexinit);
	}
	add_block(pack2, c->skexinit->payload,
		c->skexinit->rlength - c->skexinit->pad_len - 1);
	add_block(pack2, c->rkexinit->payload, c->rkexinit->rlength - 1);
	add_block(pack2, ks->payload, ks->rlength - 1);
	add_mp(pack2, c->e);
	add_mp(pack2, f);
	add_mp(pack2, k);
	sha1(pack2->payload, pack2->rlength - 1, h, nil);
	mpfree(f);

	if (c->got_sessid == 0) {
		memmove(c->sessid, h, SHA1dlen);
		c->got_sessid = 1;
	}

	q = get_string(ks, ks->payload, buf, sizeof buf, nil);
	srvkey = emalloc9p(sizeof (RSApub));
	srvkey->ek = get_mp(q);
	q += nhgetl(q) + 4;
	srvkey->n = get_mp(q);

	/*
	 * key verification is really pretty pedantic and
	 * not doing it lets us talk to ssh v1 implementations.
	 */
	if (nokeyverify)
		n = Errnone;
	else
		n = verifyhostkey(c, srvkey, sig);
	retval = -1;
	USED(retval);
	switch (n) {
#ifdef VERIFYKEYS
	case Errnokey:
		goto out;
	case Errnoverify:
		newkey = "signature verification failed; try netssh -v";
		keymbox.msg = smprint("f%04ld%s", strlen(newkey), newkey);
		break;
	case Errfactotum:
		newkey = "factotum dialogue failed; try netssh -v";
		keymbox.msg = smprint("f%04ld%s", strlen(newkey), newkey);
		break;
	case Errnone:
#else
	default:
#endif
		keymbox.msg = smprint("o0000");
		retval = 0;
		break;
	}
	nbsendul(keymbox.mchan, 1);
	if (retval == 0)
		genkeys(c, h, k);
#ifdef VERIFYKEYS
out:
#endif
	mpfree(k);
	free(ks);
	free(sig);
	free(pack2);
	free(srvkey);
	return retval;
}

static int
dh_client141(Conn *c, Packet *)
{
	Packet *p;
	mpint *e, *x;
	int n;

	/* Compute e: RFC4253 */
	e = mpnew(2048);
	x = mprand(256, genrandom, nil);
	mpexp(two, x, p14, e);
	p = new_packet(c);
	add_byte(p, SSH_MSG_KEXDH_INIT);
	add_mp(p, e);
	n = finish_packet(p);
	iowrite(c->dio, c->datafd, p->nlength, n);
	free(p);
	mpfree(e);
	mpfree(x);
	return 0;
}

static int
dh_client142(Conn *, Packet *)
{
	return 0;
}

static void
initsha1pkt(Packet *pack2, mpint *k, uchar *h)
{
	init_packet(pack2);
	add_mp(pack2, k);
	add_packet(pack2, h, SHA1dlen);
}

static void
genkeys(Conn *c, uchar h[], mpint *k)
{
	Packet *pack2;
	char buf[82], *bp, *be;			/* magic 82 */
	int n;

	pack2 = new_packet(c);
	/* Compute 40 bytes (320 bits) of keys: each alg can use what it needs */

	/* Client to server IV */
	if (debug > 1) {
		fprint(2, "k=%M\nh=", k);
		for (n = 0; n < SHA1dlen; ++n)
			fprint(2, "%02ux", h[n]);
		fprint(2, "\nsessid=");
		for (n = 0; n < SHA1dlen; ++n)
			fprint(2, "%02ux", c->sessid[n]);
		fprint(2, "\n");
	}
	initsha1pkt(pack2, k, h);
	add_byte(pack2, 'A');
	add_packet(pack2, c->sessid, SHA1dlen);
	sha1(pack2->payload, pack2->rlength - 1, c->nc2siv, nil);
	initsha1pkt(pack2, k, h);
	add_packet(pack2, c->nc2siv, SHA1dlen);
	sha1(pack2->payload, pack2->rlength - 1, c->nc2siv + SHA1dlen, nil);

	/* Server to client IV */
	initsha1pkt(pack2, k, h);
	add_byte(pack2, 'B');
	add_packet(pack2, c->sessid, SHA1dlen);
	sha1(pack2->payload, pack2->rlength - 1, c->ns2civ, nil);
	initsha1pkt(pack2, k, h);
	add_packet(pack2, c->ns2civ, SHA1dlen);
	sha1(pack2->payload, pack2->rlength - 1, c->ns2civ + SHA1dlen, nil);

	/* Client to server encryption key */
	initsha1pkt(pack2, k, h);
	add_byte(pack2, 'C');
	add_packet(pack2, c->sessid, SHA1dlen);
	sha1(pack2->payload, pack2->rlength - 1, c->nc2sek, nil);
	initsha1pkt(pack2, k, h);
	add_packet(pack2, c->nc2sek, SHA1dlen);
	sha1(pack2->payload, pack2->rlength - 1, c->nc2sek + SHA1dlen, nil);

	/* Server to client encryption key */
	initsha1pkt(pack2, k, h);
	add_byte(pack2, 'D');
	add_packet(pack2, c->sessid, SHA1dlen);
	sha1(pack2->payload, pack2->rlength - 1, c->ns2cek, nil);
	initsha1pkt(pack2, k, h);
	add_packet(pack2, c->ns2cek, SHA1dlen);
	sha1(pack2->payload, pack2->rlength - 1, c->ns2cek + SHA1dlen, nil);

	/* Client to server integrity key */
	initsha1pkt(pack2, k, h);
	add_byte(pack2, 'E');
	add_packet(pack2, c->sessid, SHA1dlen);
	sha1(pack2->payload, pack2->rlength - 1, c->nc2sik, nil);
	initsha1pkt(pack2, k, h);
	add_packet(pack2, c->nc2sik, SHA1dlen);
	sha1(pack2->payload, pack2->rlength - 1, c->nc2sik + SHA1dlen, nil);

	/* Server to client integrity key */
	initsha1pkt(pack2, k, h);
	add_byte(pack2, 'F');
	add_packet(pack2, c->sessid, SHA1dlen);
	sha1(pack2->payload, pack2->rlength - 1, c->ns2cik, nil);
	initsha1pkt(pack2, k, h);
	add_packet(pack2, c->ns2cik, SHA1dlen);
	sha1(pack2->payload, pack2->rlength - 1, c->ns2cik + SHA1dlen, nil);

	if (debug > 1) {
		be = buf + sizeof buf;
		fprint(2, "Client to server IV:\n");
		for (n = 0, bp = buf; n < SHA1dlen*2; ++n)
			bp = seprint(bp, be, "%02x", c->nc2siv[n]);
		fprint(2, "%s\n", buf);

		fprint(2, "Server to client IV:\n");
		for (n = 0, bp = buf; n < SHA1dlen*2; ++n)
			bp = seprint(bp, be, "%02x", c->ns2civ[n]);
		fprint(2, "%s\n", buf);

		fprint(2, "Client to server EK:\n");
		for (n = 0, bp = buf; n < SHA1dlen*2; ++n)
			bp = seprint(bp, be, "%02x", c->nc2sek[n]);
		fprint(2, "%s\n", buf);

		fprint(2, "Server to client EK:\n");
		for (n = 0, bp = buf; n < SHA1dlen*2; ++n)
			bp = seprint(bp, be, "%02x", c->ns2cek[n]);
		fprint(2, "%s\n", buf);
	}
	free(pack2);
}

Kex dh1sha1 = {
	"diffie-hellman-group1-sha1",
	dh_server1,
	dh_client11,
	dh_client12
};

Kex dh14sha1 = {
	"diffie-hellman-group14-sha1",
	dh_server14,
	dh_client141,
	dh_client142
};

PKA rsa_pka = {
	"ssh-rsa",
	rsa_ks,
	rsa_sign,
	rsa_verify
};

PKA dss_pka = {
	"ssh-dss",
	dss_ks,
	dss_sign,
	dss_verify
};
