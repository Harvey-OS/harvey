/* Copyright © 2013-2014 David Hoskin <root@davidrhoskin.com> */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include <bio.h>
#include <mp.h>
#include <libsec.h>
#include <auth.h>
#include "httpd.h"
#include "httpsrv.h"

enum
{
	/* misc parameters */
	MAXHDRS = 64,
	STACKSZ = 32768,
	BUFSZ = 16384,
	CHANBUF = 8,

	/* packet types */
	/* standard non-control frames */
	Cont = 0x0,
	Text = 0x1,
	Binary = 0x2,
	/* reserved non-control frames */
	/* standard control frames */
	Close = 0x8,
	Ping = 0x9,
	Pong = 0xA,
	/* reserved control frames */
};

typedef struct Procio Procio;
struct Procio
{
	Channel *c;
	Biobuf *b;
	int fd;
	char **argv;
};

typedef struct Buf Buf;
struct Buf
{
	uchar *buf;
	long n;
};

typedef struct Wspkt Wspkt;
struct Wspkt
{
	Buf;
	int type;
};

/* XXX The default was not enough, but this is just a guess. at least 2*sizeof Biobuf */
int mainstacksize = 128*1024;

const char wsnoncekey[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const char wsversion[] = "13";

HSPairs *
parseheaders(char *headers)
{
	char *hdrlines[MAXHDRS], *kv[2];
	HSPairs *h, *t, *tmp;
	int nhdr;
	int i;

	h = t = nil;

	nhdr = getfields(headers, hdrlines, MAXHDRS, 1, "\r\n");

	/*
	* XXX I think leading whitespaces signifies a continuation line.
	* Skip the first line, or else getfields(..., " ") picks up the GET.
	*/
	for(i = 1; i < nhdr; ++i){

		if(hdrlines[i] == nil)
			continue;

		getfields(hdrlines[i], kv, 2, 1, ": \t");

		tmp = malloc(sizeof(HSPairs));
		if(tmp == nil)
			goto cleanup;

		tmp->s = kv[0];
		tmp->t = kv[1];

		if(h == nil){
			h = t = tmp;
		}else{
			t->next = tmp;
			t = tmp;
		}
		tmp->next = nil;
	}

	return h;

cleanup:
	for(t = h->next; h != nil; h = t, t = h->next)
		free(h);
	return nil;
}

char *
getheader(HSPairs *h, const char *k)
{
	for(; h != nil; h = h->next)
		if(cistrcmp(h->s, k) == 0)
			return h->t;
	return nil;
}

int
failhdr(HConnect *c, int code, const char *status, const char *message)
{
	Hio *o;

	o = &c->hout;
	hprint(o, "%s %d %s\r\n", hversion, code, status);
	hprint(o, "Server: Plan9\r\n");
	hprint(o, "Date: %D\r\n", time(nil));
	hprint(o, "Content-type: text/html\r\n");
	hprint(o, "\r\n");
	hprint(o, "<html><head><title>%d %s</title></head>\n", code, status);
	hprint(o, "<body><h1>%d %s</h1>\n", code, status);
	hprint(o, "<p>Failed to establish websocket connection: %s\n", message);
	hprint(o, "</body></html>\n");
	hflush(o);
	return 0;
}

void
okhdr(HConnect *c, const char *wshashedkey, const char *proto)
{
	Hio *o;

	o = &c->hout;
	hprint(o, "%s 101 Switching Protocols\r\n", hversion);
	hprint(o, "Upgrade: websocket\r\n");
	hprint(o, "Connection: upgrade\r\n");
	hprint(o, "Sec-WebSocket-Accept: %s\r\n", wshashedkey);
	if(proto != nil)
		hprint(o, "Sec-WebSocket-Protocol: %s\r\n", proto);
	/* we don't handle extensions */
	hprint(o, "\r\n");
	hflush(o);
}

int
testwsversion(const char *vs)
{
	int i, n;
	char *v[16];

	n = getfields(vs, v, 16, 1, "\t ,");
	for(i = 0; i < n; ++i)
		if(strcmp(v[i], wsversion) == 0)
			return 1;
	return 0;
}

uvlong
getbe(uchar *t, int w)
{
	uint i;
	uvlong r;

	r = 0;
	for(i = 0; i < w; i++)
		r = r<<8 | t[i];
	return r;
}

int
Bgetbe(Biobuf *b, uvlong *u, int sz)
{
	uchar buf[8];

	if(Bread(b, buf, sz) != sz)
		return -1;

	*u = getbe(buf, sz);
	return 1;
}

int
sendpkt(Biobuf *b, Wspkt *pkt)
{
	uchar hdr[2+8];
	long hdrsz, len;

	hdr[0] = 0x80 | pkt->type;
	len = pkt->n;

	/* XXX should use putbe(). */
	if(len >= (1 << 16)){
		hdrsz = 2 + 8;
		hdr[1] = 127;
		hdr[2] = hdr[3] = hdr[4] = hdr[5] = 0;
		hdr[6] = len >> 24;
		hdr[7] = len >> 16;
		hdr[8] = len >> 8;
		hdr[9] = len >> 0;
	}else if(len >= 126){
		hdrsz = 2 + 2;
		hdr[1] = 126;
		hdr[2] = len >> 8;
		hdr[3]= len >> 0;
	}else{
		hdrsz = 2;
		hdr[1] = len;
	}

	if(Bwrite(b, hdr, hdrsz) != hdrsz)
		return -1;
	if(Bwrite(b, pkt->buf, len) != len)
		return -1;
	if(Bflush(b) < 0)
		return -1;

	return 0;
}

int
recvpkt(Wspkt *pkt, Biobuf *b)
{
	long x;
	int masked;
	uchar mask[4];

	pkt->type = Bgetc(b);
	if(pkt->type < 0){
		return -1;
	}
	/* Strip FIN/continuation bit. */
	pkt->type &= 0x0F;

	pkt->n = Bgetc(b);
	if(pkt->n < 0){
		return -1;
	}
	masked = pkt->n & 0x80;
	pkt->n &= 0x7F;

	if(pkt->n >= 127){
		if(Bgetbe(b, (uvlong *)&pkt->n, 8) != 1)
			return -1;
	}else if(pkt->n == 126){
		if(Bgetbe(b, (uvlong *)&pkt->n, 2) != 1)
			return -1;
	}

	if(masked){
		if(Bread(b, mask, 4) != 4)
			return -1;
	}
	/* allocate appropriate buffer */
	if(pkt->n > BUFSZ){
		/*
		* buffer is unacceptably large!
		* XXX this should close the connection with a specific error code.
		* See websocket spec.
		*/
		return -1;
	}else if(pkt->n == 0){
		pkt->buf = nil;
		return 1;
	}else{
		pkt->buf = malloc(pkt->n);
		if(pkt->buf == nil)
			return -1;

		if(Bread(b, pkt->buf, pkt->n) != pkt->n){
			free(pkt->buf);
			return -1;
		}

		if(masked)
			for(x = 0; x < pkt->n; ++x)
				pkt->buf[x] ^= mask[x % 4];

		return 1;
	}
}

void
wsreadproc(void *arg)
{
	Procio *pio;
	Channel *c;
	Biobuf *b;
	Wspkt pkt;

	pio = (Procio *)arg;
	c = pio->c;
	b = pio->b;

	for(;;){
		if(recvpkt(&pkt, b) < 0)
			break;
		if(send(c, &pkt) < 0){
			free(pkt.buf);
			break;
		}
	}

	chanclose(c);
	threadexits(nil);
}

void
wswriteproc(void *arg)
{
	Procio *pio;
	Channel *c;
	Biobuf *b;
	Wspkt pkt;

	pio = (Procio *)arg;
	c = pio->c;
	b = pio->b;

	for(;;){
		if(recv(c, &pkt) < 0)
			break;
		if(sendpkt(b, &pkt) < 0){
			free(pkt.buf);
			break;
		}
		free(pkt.buf);
	}

	chanclose(c);
	threadexits(nil);
}

void
pipereadproc(void *arg)
{
	Procio *pio;
	Channel *c;
	int fd;
	Buf b;

	pio = (Procio *)arg;
	c = pio->c;
	fd = pio->fd;

	for(;;){
		b.buf = malloc(BUFSZ);
		if(b.buf == nil)
			break;
		b.n = read(fd, b.buf, BUFSZ);
		if(b.n < 1)
			break;
		if(send(c, &b) < 0)
			break;
	}

	free(b.buf);
	chanclose(c);
	threadexits(nil);
}

void
pipewriteproc(void *arg)
{
	Procio *pio;
	Channel *c;
	int fd;
	Buf b;

	pio = (Procio *)arg;
	c = pio->c;
	fd = pio->fd;

	for(;;){
		if(recv(c, &b) != 1)
			break;
		if(write(fd, b.buf, b.n) != b.n){
			free(b.buf);
			break;
		}
		free(b.buf);
	}

	chanclose(c);
	threadexits(nil);
}

void
mountproc(void *arg)
{
	Procio *pio;
	int fd, i;
	char **argv;

	pio = (Procio *)arg;
	fd = pio->fd;
	argv = pio->argv;

	for(i = 0; i < 20; ++i){
		if(i != fd)
			close(i);
	}

	newns("none", nil);

	if(mount(fd, -1, "/dev/", MBEFORE, "") == -1)
		sysfatal("mount failed: %r");

	procexec(nil, argv[0], argv);
}

void
echoproc(void *arg)
{
	Procio *pio;
	int fd;
	char buf[1024];
	int n;

	pio = (Procio *)arg;
	fd = pio->fd;

	for(;;){
		n = read(fd, buf, 1024);
		if(n > 0)
			write(fd, buf, n);
	}
}

int
wscheckhdr(HConnect *c)
{
	HSPairs *hdrs;
	char *s, *wsclientkey;
	char *rawproto;
	char *proto;
	char wscatkey[64];
	uchar wshashedkey[SHA1dlen];
	char wsencoded[32];

	if(strcmp(c->req.meth, "GET") != 0)
		return hunallowed(c, "GET");

	//return failhdr(c, 403, "Forbidden", "my hair is on fire");

	hdrs = parseheaders((char *)c->header);

	s = getheader(hdrs, "upgrade");
	if(s == nil || !cistrstr(s, "websocket"))
		return failhdr(c, 400, "Bad Request", "no <code>upgrade: websocket</code> header.");
	s = getheader(hdrs, "connection");
	if(s == nil || !cistrstr(s, "upgrade"))
		return failhdr(c, 400, "Bad Request", "no <code>connection: upgrade</code> header.");
	wsclientkey = getheader(hdrs, "sec-websocket-key");
	if(wsclientkey == nil || strlen(wsclientkey) != 24)
		return failhdr(c, 400, "Bad Request", "invalid websocket nonce key.");
	s = getheader(hdrs, "sec-websocket-version");
	if(s == nil || !testwsversion(s))
		return failhdr(c, 426, "Upgrade Required", "could not match websocket version.");
	/* XXX should get resource name */
	rawproto = getheader(hdrs, "sec-websocket-protocol");
	proto = rawproto;
	/* XXX should test if proto is acceptable" */
	/* should get sec-websocket-extensions */

	/* OK, we seem to have a valid Websocket request. */

	/* Hash websocket key. */
	strcpy(wscatkey, wsclientkey);
	strcat(wscatkey, wsnoncekey);
	sha1((uchar *)wscatkey, strlen(wscatkey), wshashedkey, nil);
	enc64(wsencoded, 32, wshashedkey, SHA1dlen);

	okhdr(c, wsencoded, proto);
	hflush(&c->hout);

	/* We should now have an open Websocket connection. */

	return 1;
}

int
dowebsock(void)
{
	Biobuf bin, bout;
	Wspkt pkt;
	Buf buf;
	int p[2];
	Alt a[] = {
	/*	c	v	op */
		{nil, &pkt, CHANRCV},
		{nil, &buf, CHANRCV},
		{nil, nil, CHANEND},
	};
	Procio fromws, tows, frompipe, topipe;
	Procio mountp, echop;
	char *argv[] = {"/bin/rc", "-c", "ramfs && exec acme", nil};

	fromws.c = chancreate(sizeof(Wspkt), CHANBUF);
	tows.c = chancreate(sizeof(Wspkt), CHANBUF);
	frompipe.c = chancreate(sizeof(Buf), CHANBUF);
	topipe.c = chancreate(sizeof(Buf), CHANBUF);

	a[0].c = fromws.c;
	a[1].c = frompipe.c;

	Binit(&bin, 0, OREAD);
	Binit(&bout, 1, OWRITE);
	fromws.b = &bin;
	tows.b = &bout;

	pipe(p);
	//fd = create("/srv/weebtest", OWRITE, 0666);
	//fprint(fd, "%d", p[0]);
	//close(fd);
	//close(p[0]);

	frompipe.fd = p[1];
	topipe.fd = p[1];

	mountp.fd = echop.fd = p[0];
	mountp.argv = argv;

	proccreate(wsreadproc, &fromws, STACKSZ);
	proccreate(wswriteproc, &tows, STACKSZ);
	proccreate(pipereadproc, &frompipe, STACKSZ);
	proccreate(pipewriteproc, &topipe, STACKSZ);

	//proccreate(echoproc, &echop, STACKSZ);
	procrfork(mountproc, &mountp, STACKSZ, RFNAMEG|RFFDG);

	for(;;){
		int i;

		i = alt(a);
		if(chanclosing(a[i].c) >= 0){
			a[i].op = CHANNOP;
			pkt.type = Close;
			pkt.buf = nil;
			pkt.n = 0;
			send(tows.c, &pkt);
			goto done;
		}

		switch(i){
		case 0: /* from socket */
			if(pkt.type == Ping){
				pkt.type = Pong;
				send(tows.c, &pkt);
			}else if(pkt.type == Close){
				send(tows.c, &pkt);
				goto done;
			}else{
				send(topipe.c, &pkt.Buf);
			}
			break;
		case 1: /* from pipe */
			pkt.type = Binary;
			pkt.Buf = buf;
			send(tows.c, &pkt);
			break;
		default:
			sysfatal("can't happen");
		}
	}
done:
	return 1;
}

void
threadmain(int argc, char **argv)
{
	HConnect *c;

	c = init(argc, argv);
	if(hparseheaders(c, HSTIMEOUT) >= 0)
		if(wscheckhdr(c) >= 0)
			dowebsock();

	threadexitsall(nil);
}
