#include <u.h>
#include <libc.h>
#include <mp.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <libsec.h>
#include <ip.h>
#include "netssh.h"

extern Cipher *cryptos[];

Packet *
new_packet(Conn *c)
{
	Packet *p;

	p = emalloc9p(sizeof(Packet));
	init_packet(p);
	p->c = c;
	return p;
}

void
init_packet(Packet *p)
{
	memset(p, 0, sizeof(Packet));
	p->rlength = 1;
}

void
add_byte(Packet *p, char c)
{
	p->payload[p->rlength-1] = c;
	p->rlength++;
}

void
add_uint32(Packet *p, ulong l)
{
	hnputl(p->payload+p->rlength-1, l);
	p->rlength += 4;
}

ulong
get_uint32(Packet *, uchar **data)
{
	ulong x;
	x = nhgetl(*data);
	*data += 4;
	return x;
}

int
add_packet(Packet *p, void *data, int len)
{
	if(p->rlength + len > Maxpayload)
		return -1;
	memmove(p->payload + p->rlength - 1, data, len);
	p->rlength += len;
	return 0;
}

void
add_block(Packet *p, void *data, int len)
{
	hnputl(p->payload + p->rlength - 1, len);
	p->rlength += 4;
	add_packet(p, data, len);
}

void 
add_string(Packet *p, char *s)
{
	uchar *q;
	int n;
	uchar nn[4];

	n = strlen(s);
	hnputl(nn, n);
	q = p->payload + p->rlength - 1;
	memmove(q, nn, 4);
	memmove(q+4, s, n);
	p->rlength += n + 4;
}

uchar *
get_string(Packet *p, uchar *q, char *s, int lim, int *len)
{
	int n, m;

	if (p && q > p->payload + p->rlength)
		s[0] = '\0';
	m = nhgetl(q);
	q += 4;
	if(m < lim)
		n = m;
	else
		n = lim - 1;
	memmove(s, q, n);
	s[n] = '\0';
	q += m;
	if(len)
		*len = n;
	return q;
}

void
add_mp(Packet *p, mpint *x)
{
	uchar *q;
	int n;

	q = p->payload + p->rlength - 1;
	n = mptobe(x, q + 4, Maxpktpay - p->rlength + 1 - 4, nil);
	if(q[4] & 0x80){
		memmove(q + 5, q + 4, n);
		q[4] = 0;
		n++;
	}
	hnputl(q, n);
	p->rlength += n + 4;
}

mpint *
get_mp(uchar *q)
{
	return betomp(q + 4, nhgetl(q), nil);
}

int
finish_packet(Packet *p)
{
	Conn *c;
	uchar *q, *buf;
	int blklen, i, n2, n1, maclen;

	c = p->c;
	blklen = 8;
	if(c && debug > 1)
		fprint(2, "%s: in finish_packet: enc %d outmac %d len %ld\n",
			argv0, c->encrypt, c->outmac, p->rlength);
	if(c && c->encrypt != -1){
		blklen = cryptos[c->encrypt]->blklen;
		if(blklen < 8)
			blklen = 8;
	}
	n1 = p->rlength - 1;
	n2 = blklen - (n1 + 5) % blklen;
	if(n2 < 4)
		n2 += blklen;
	p->pad_len = n2;
	for(i = 0, q = p->payload + n1; i < n2; ++i, ++q)
		*q = fastrand();
	p->rlength = n1 + n2 + 1;
	hnputl(p->nlength, p->rlength);
	maclen = 0;
	if(c && c->outmac != -1){
		maclen = SHA1dlen;
		buf = emalloc9p(Maxpktpay);
		hnputl(buf, c->outseq);
		memmove(buf + 4, p->nlength, p->rlength + 4);
		hmac_sha1(buf, p->rlength + 8, c->outik, maclen, q, nil);
		free(buf);
	}
	if(c && c->encrypt != -1)
		cryptos[c->encrypt]->encrypt(c->enccs, p->nlength, p->rlength + 4);
	if (c)
		c->outseq++;
	if(debug > 1)
		fprint(2, "%s: leaving finish packet: len %ld n1 %d n2 %d maclen %d\n",
			argv0, p->rlength, n1, n2, maclen);
	return p->rlength + 4 + maclen;
}

/*
 * The first blklen bytes are already decrypted so we could find the
 * length.
 */
int
undo_packet(Packet *p)
{
	Conn *c;
	long nlength;
	int nb;
	uchar rmac[SHA1dlen], *buf;

	c = p->c;
	nb = 4;
	if(c->decrypt != -1)
		nb = cryptos[c->decrypt]->blklen;
	if(c->inmac != -1)
		p->rlength -= SHA1dlen;			/* was magic 20 */
	nlength = nhgetl(p->nlength);
	if(c->decrypt != -1)
		cryptos[c->decrypt]->decrypt(c->deccs, p->nlength + nb,
			p->rlength + 4 - nb);
	if(c->inmac != -1){
		buf = emalloc9p(Maxpktpay);
		hnputl(buf, c->inseq);
		memmove(buf + 4, p->nlength, nlength + 4);
		hmac_sha1(buf, nlength + 8, c->inik, SHA1dlen, rmac, nil);
		free(buf);
		if(memcmp(rmac, p->payload + nlength - 1, SHA1dlen) != 0){
			fprint(2, "%s: received MAC verification failed: seq=%d\n",
				argv0, c->inseq);
			return -1;
		}
	}
	c->inseq++;
	p->rlength -= p->pad_len;
	p->pad_len = 0;
	return p->rlength - 1;
}

void
dump_packet(Packet *p)
{
	int i;
	char *buf, *q, *e;

	fprint(2, "Length: %ld, Padding length: %d\n", p->rlength, p->pad_len);
	q = buf = emalloc9p(Copybufsz);
	e = buf + Copybufsz;
	for(i = 0; i < p->rlength - 1; ++i){
		q = seprint(q, e, " %02x", p->payload[i]);
		if(i % 16 == 15)
			q = seprint(q, e, "\n");
		if(q - buf > Copybufsz - 4){
			fprint(2, "%s", buf);
			q = buf;
		}
	}
	fprint(2, "%s\n", buf);
	free(buf);
}
