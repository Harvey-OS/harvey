/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <mp.h>
#include <libsec.h>
#include "SConn.h"

extern int verbose;

typedef struct ConnState {
	uint8_t secret[SHA1dlen];
	uint32_t seqno;
	RC4state rc4;
} ConnState;

typedef struct SS {
	int fd;  // file descriptor for read/write of encrypted data
	int alg; // if nonzero, "alg sha rc4_128"
	ConnState in, out;
} SS;

static int
SC_secret(SConn* conn, uint8_t* sigma, int direction)
{
	SS* ss = (SS*)(conn->chan);
	int nsigma = conn->secretlen;

	if(direction != 0) {
		hmac_sha1(sigma, nsigma, (uint8_t*)"one", 3, ss->out.secret,
		          nil);
		hmac_sha1(sigma, nsigma, (uint8_t*)"two", 3, ss->in.secret,
		          nil);
	} else {
		hmac_sha1(sigma, nsigma, (uint8_t*)"two", 3, ss->out.secret,
		          nil);
		hmac_sha1(sigma, nsigma, (uint8_t*)"one", 3, ss->in.secret,
		          nil);
	}
	setupRC4state(&ss->in.rc4, ss->in.secret, 16); // restrict to 128 bits
	setupRC4state(&ss->out.rc4, ss->out.secret, 16);
	ss->alg = 1;
	return 0;
}

static void
hash(uint8_t secret[SHA1dlen], uint8_t* data, int len, int seqno,
     uint8_t d[SHA1dlen])
{
	DigestState sha;
	uint8_t seq[4];

	seq[0] = seqno >> 24;
	seq[1] = seqno >> 16;
	seq[2] = seqno >> 8;
	seq[3] = seqno;
	memset(&sha, 0, sizeof sha);
	sha1(secret, SHA1dlen, nil, &sha);
	sha1(data, len, nil, &sha);
	sha1(seq, 4, d, &sha);
}

static int
verify(uint8_t secret[SHA1dlen], uint8_t* data, int len, int seqno,
       uint8_t d[SHA1dlen])
{
	DigestState sha;
	uint8_t seq[4];
	uint8_t digest[SHA1dlen];

	seq[0] = seqno >> 24;
	seq[1] = seqno >> 16;
	seq[2] = seqno >> 8;
	seq[3] = seqno;
	memset(&sha, 0, sizeof sha);
	sha1(secret, SHA1dlen, nil, &sha);
	sha1(data, len, nil, &sha);
	sha1(seq, 4, digest, &sha);
	return memcmp(d, digest, SHA1dlen);
}

static int
SC_read(SConn* conn, uint8_t* buf, int n)
{
	SS* ss = (SS*)(conn->chan);
	uint8_t count[2], digest[SHA1dlen];
	int len, nr;

	if(read(ss->fd, count, 2) != 2 || (count[0] & 0x80) == 0) {
		snprint((char*)buf, n, "!SC_read invalid count");
		return -1;
	}
	len = (count[0] & 0x7f) << 8 | count[1]; // SSL-style count; no pad
	if(ss->alg) {
		len -= SHA1dlen;
		if(len <= 0 || readn(ss->fd, digest, SHA1dlen) != SHA1dlen) {
			snprint((char*)buf, n, "!SC_read missing sha1");
			return -1;
		}
		if(len > n || readn(ss->fd, buf, len) != len) {
			snprint((char*)buf, n, "!SC_read missing data");
			return -1;
		}
		rc4(&ss->in.rc4, digest, SHA1dlen);
		rc4(&ss->in.rc4, buf, len);
		if(verify(ss->in.secret, buf, len, ss->in.seqno, digest) != 0) {
			snprint((char*)buf, n,
			        "!SC_read integrity check failed");
			return -1;
		}
	} else {
		if(len <= 0 || len > n) {
			snprint((char*)buf, n,
			        "!SC_read implausible record length");
			return -1;
		}
		if((nr = readn(ss->fd, buf, len)) != len) {
			snprint((char*)buf, n,
			        "!SC_read expected %d bytes, but got %d", len,
			        nr);
			return -1;
		}
	}
	ss->in.seqno++;
	return len;
}

static int
SC_write(SConn* conn, uint8_t* buf, int n)
{
	SS* ss = (SS*)(conn->chan);
	uint8_t count[2], digest[SHA1dlen], enc[Maxmsg + 1];
	int len;

	if(n <= 0 || n > Maxmsg + 1) {
		werrstr("!SC_write invalid n %d", n);
		return -1;
	}
	len = n;
	if(ss->alg)
		len += SHA1dlen;
	count[0] = 0x80 | len >> 8;
	count[1] = len;
	if(write(ss->fd, count, 2) != 2) {
		werrstr("!SC_write invalid count");
		return -1;
	}
	if(ss->alg) {
		hash(ss->out.secret, buf, n, ss->out.seqno, digest);
		rc4(&ss->out.rc4, digest, SHA1dlen);
		memcpy(enc, buf, n);
		rc4(&ss->out.rc4, enc, n);
		if(write(ss->fd, digest, SHA1dlen) != SHA1dlen ||
		   write(ss->fd, enc, n) != n) {
			werrstr("!SC_write error on send");
			return -1;
		}
	} else {
		if(write(ss->fd, buf, n) != n) {
			werrstr("!SC_write error on send");
			return -1;
		}
	}
	ss->out.seqno++;
	return n;
}

static void
SC_free(SConn* conn)
{
	SS* ss = (SS*)(conn->chan);

	close(ss->fd);
	free(ss);
	free(conn);
}

SConn*
newSConn(int fd)
{
	SS* ss;
	SConn* conn;

	if(fd < 0)
		return nil;
	ss = (SS*)emalloc(sizeof(*ss));
	conn = (SConn*)emalloc(sizeof(*conn));
	ss->fd = fd;
	ss->alg = 0;
	conn->chan = (void*)ss;
	conn->secretlen = SHA1dlen;
	conn->free = SC_free;
	conn->secret = SC_secret;
	conn->read = SC_read;
	conn->write = SC_write;
	return conn;
}

void
writerr(SConn* conn, char* s)
{
	char buf[Maxmsg];

	snprint(buf, Maxmsg, "!%s", s);
	conn->write(conn, (uint8_t*)buf, strlen(buf));
}

int
readstr(SConn* conn, char* s)
{
	int n;

	n = conn->read(conn, (uint8_t*)s, Maxmsg);
	if(n >= 0) {
		s[n] = 0;
		if(s[0] == '!') {
			memmove(s, s + 1, n);
			n = -1;
		}
	} else {
		strcpy(s, "connection read error");
	}
	return n;
}
