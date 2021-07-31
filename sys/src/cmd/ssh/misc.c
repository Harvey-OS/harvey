#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>

#include "ssh.h"

char	*cipher_names[] = {
	"No encryption",
	"IDEA",
	"DES",
	"Triple DES",
	"TSS",
	"RC4",
	0
};

char	*auth_names[] = {
	"Illegal",
	".rhosts or /etc/hosts.equiv",
	"pure RSA",
	"Password",
	".shosts with RSA",
	0
};

int doabort;
void
error(char *fmt, ...) {
	va_list arg;
	char buf[2048];

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	fprint(2, "%s: %s\n", progname, buf);
	if(doabort)
		abort();
	exits("error");
}


/* #define DEBUG (DBG_SCP) */
int DEBUG;

void
debug(int level, char *fmt, ...) {
	va_list arg;
	char buf[2048];

	if (level & DEBUG) {
		va_start(arg, fmt);
		doprint(buf, buf+sizeof(buf), fmt, arg);
		va_end(arg);
		fprint(2, "%s", buf);
	}
}

void
debugbig(int level, mpint *b, char term) {
	uchar buf[2048];
	int s, i;

	if (level & DEBUG) {
		s = mptobe(b, buf, 1024, nil);
		for (i = 0; i < s; i++)
			fprint(2, "%2.2x", buf[i]);
		if (term)
			fprint(2, "%c", term);
	}
}

void *
zalloc(int size) {
	void *x;
	/* allocate and set to zero */

	if ((x = mallocz(size, 1)) == 0)
		error("malloc");
	setmalloctag(x, getcallerpc(&size));
	return x;
}

void *
mmalloc(int size) {
	void *x;

	x = zalloc(size);
	setmalloctag(x, getcallerpc(&size));
	return x;
}

void
mfree(Packet *packet) {

	debug(DBG_PACKET, "Remaining Length: %d\n", packet->length);
	free(packet);
}

void
printbig(int fd, mpint *b, int base, char term)
{
	char *p;

	p = mptoa(b, base, nil, 0);
	if(term)
		fprint(fd, "%s%c", p, term);
	else
		fprint(fd, "%s", p);
	free(p);
}


void
printpublickey(int fd, RSApub *key) {

	fprint(fd, "%d ", mpsignif(key->n));
	printbig(fd, key->ek, 16, ' ');
	printbig(fd, key->n, 16, '\n');
}

void
printdecpublickey(int fd, RSApub *key) {
	fprint(fd, "%d ", mpsignif(key->n));
	printbig(fd, key->ek, 10, ' ');
	printbig(fd, key->n, 10, '\n');
}

int
readpublickey(FILE *f, RSApub *key) {
	char buf[2048];
	char *p;
	mpint *ek, *n;

	if (fgets(buf, 2048, f) == nil)
		return 0;

	/* 
	 * hostname size exponent modulus
	 * but the hostname gets cut out before we get called.
	 */
	ek = n = nil;

	if((p = strchr(buf, ' ')) == nil
	|| (ek=strtomp(p, &p, 16, key->ek)) == nil
	|| (n=strtomp(p, &p, 16, key->n)) == nil){
		if(ek != key->ek)
			mpfree(ek);
		if(n != key->n)
			mpfree(n);
		return 0;
	}

	key->ek = ek;
	key->n = n;
	return 1;
}

void
printsecretkey(int fd, RSApriv *key) {

	fprint(fd, "%d ", mpsignif(key->pub.n));
	printbig(fd, key->pub.ek, 16, ' ');
	printbig(fd, key->dk, 16, ' ');
	printbig(fd, key->pub.n, 16, ' ');
	printbig(fd, key->p, 16, ' ');
	printbig(fd, key->q, 16, ' ');
	printbig(fd, key->kp, 16, ' ');
	printbig(fd, key->kq, 16, ' ');
	printbig(fd, key->c2, 16, '\n');
}

int
readsecretkey(FILE *f, RSApriv *key) {
	char buf[2048];
	char *p;
	mpint *ek, *dk, *n, *pp, *q, *kp, *kq, *c2;

	if (fgets(buf, 2048, f) == nil)
		return 0;

	/*
	 * size ek dk n p q kp kq c2
	 */

	ek = dk = n = pp = q = kp = kq = c2 = nil;

	if((p = strchr(buf, ' ')) == nil
	|| (ek=strtomp(p, &p, 16, key->pub.ek)) == nil
	|| (dk=strtomp(p, &p, 16, key->dk)) == nil
	|| (n=strtomp(p, &p, 16, key->pub.n)) == nil
	|| (pp=strtomp(p, &p, 16, key->p)) == nil
	|| (q=strtomp(p, &p, 16, key->q)) == nil
	|| (kp=strtomp(p, &p, 16, key->kp)) == nil
	|| (kq=strtomp(p, &p, 16, key->kq)) == nil
	|| (c2=strtomp(p, &p, 16, key->c2)) == nil){
		if(ek != key->pub.ek)
			mpfree(ek);
		if(dk != key->dk)
			mpfree(dk);
		if(n != key->pub.n)
			mpfree(n);
		if(pp != key->p)
			mpfree(pp);
		if(q != key->q)
			mpfree(q);
		if(kp != key->p)
			mpfree(kp);
		if(kq != key->q)
			mpfree(kq);
		if(c2 != key->c2)
			mpfree(c2);
		return 0;
	}

	key->pub.ek = ek;
	key->dk = dk;
	key->pub.n = n;
	key->p = pp;
	key->q = q;
	key->kp = kp;
	key->kq = kq;
	key->c2 = c2;
	return 1;
}

void
comp_sess_id(mpint *m1, mpint *m2, uchar *cookie, uchar *sid) {
	uchar sess_str[1024];
	int bytes;

	bytes = mptobe(m1, sess_str, sizeof(sess_str)-8, nil);
	bytes += mptobe(m2, sess_str + bytes, sizeof(sess_str)-8-bytes, nil);

	memcpy(sess_str + bytes, cookie, 8);
	md5(sess_str, bytes+8, sid, 0);
}

void
RSApad(uchar *inbuf, int inbytes, uchar *outbuf, int outbytes) {
	uchar *p, *q;
	uchar x;

	debug(DBG_CRYPTO, "Padding a %d-byte number to %d bytes\n",
		inbytes, outbytes);

	assert(outbytes >= inbytes+3);
	p = &inbuf[inbytes-1];
	q = &outbuf[outbytes-1];
	while (p >= inbuf) *q-- = *p--;
	*q-- = 0;
	while (q >= outbuf+2) {
		while ((x = nrand(0x100)) == 0)
			;
		*q-- = x;
	}
	outbuf[0] = 0;
	outbuf[1] = 2;
}

mpint*
RSAunpad(mpint *bin, int bytes) {
	int n;
	uchar buf[256];
	mpint *bout;

	bout = mpnew(0);
	setmalloctag(bout, getcallerpc(&bin));
	n = mptobe(bin, buf, 256, nil);
	if (buf[0] != 2)
		error("RSAunpad");
	betomp(buf+n-bytes, bytes, bout);
	return bout;
}

mpint*
RSAEncrypt(mpint *in, RSApub *rsa)
{
	mpint *out;

	out = rsaencrypt(rsa, in, nil);
	mpfree(in);
	return out;
}

mpint*
RSADecrypt(mpint *in, RSApriv *rsa)
{
	mpint *out;

	out = rsadecrypt(rsa, in, nil);
	setmalloctag(out, getcallerpc(&in));
	mpfree(in);
	return out;
}

int
verify_key(char *host, RSApub *received_key, char *keyring) {
	FILE *f;
	RSApub	*file_key;
	char s[2048], *p;
	int c;

/*	Return values:	key_ok, key_wrong, key_notfound, key_file */

	if ((f = fopen(keyring, "r")) == nil) {
		debug(DBG_AUTH, "Can't open %s: %r\n", keyring);	
		return key_file;
	}
	for (;;) {
		p = s;
		while ((c = fgetc(f)) != '\n') {
			if (c == EOF) {
				fclose(f);
				return key_notfound;
			}
			if (p == s && c == '#') {
				fgets(s, 2048, f);
				break;
			}
			if (c == ' ' || c == '\t') {
				*p = 0;
				if (strcmp(s, host)) {
					debug(DBG_AUTH, "wrong host %s\n", s);	
					fgets(s, 2048, f);
					break;
				}
				debug(DBG_AUTH, "host found: %s\n", s);
				file_key = rsapuballoc();
				if (readpublickey(f, file_key) == 0) {
					debug(DBG_AUTH, "Can't read key\n");	
					fclose(f);
					rsapubfree(file_key);
					return key_file;
				}
				if (mpcmp(file_key->n, received_key->n) ||
				    mpcmp(file_key->ek, received_key->ek)) {
					debug(DBG_AUTH, "wrong key\n", s);	
					fclose(f);
					rsapubfree(file_key);
					return key_wrong;
				}
				fclose(f);
				rsapubfree(file_key);
				return key_ok;
			}
			*p++ = c;
		}
	}
	debug(DBG_AUTH, "Host not found\n");	
	fclose(f);
	return key_notfound;
}

int
replace_key(char *host, RSApub *received_key, char *keyring) {
	FILE *f;
	int fd;
	char s[1024], keyringnew[256], *p;
	int i, done;
	Dir dir;

/*	Return values:	key_ok, key_file */

	if ((f = fopen(keyring, "r")) == nil) {
		debug(DBG_AUTH, "Can't open %s: %r\n", keyring);	
		return key_file;
	}
	snprint(keyringnew, sizeof(keyringnew), "%s.new", keyring);
	if ((fd = create(keyringnew, OWRITE|OTRUNC, 0644)) < 0) {
		debug(DBG_AUTH, "Can't open %s: %r\n", keyringnew);	
		fclose(f);
		return key_file;
	}
	done = 0;
	while (fgets(s, sizeof(s), f)) {
		i = strcspn(s, " \t\n");
		if (strlen(host) == i && strncmp(s, host, i) == 0) {
			if (done == 0) {
				fprint(fd, "%s ", host);
				printpublickey(fd, received_key);
				done = 1;
			}
		} else {
			write(fd, s, strlen(s));
		}
	}
	if (done == 0) {
		fprint(fd, "%s ", host);
		printpublickey(fd, received_key);
	}
	fclose(f);
	remove(keyring);
	dirfstat(fd, &dir);
	p = strrchr(dir.name, '.');
	if (p == 0) error("There's something rotten in the State of Denmark");
	*p = 0;
	dirfwstat(fd, &dir);
	close(fd);
	return key_ok;
}

int
isatty(int fd) {
	Dir d1, d2;

	if(dirfstat(fd, &d1)==-1) return 0;
	if(dirstat("/dev/cons", &d2)==-1) return 0;
	return d1.type==d2.type&&d1.dev==d2.dev&&d1.qid.path==d2.qid.path;
}

void
getdomainname(char *name, char *dest, int len) {
	char *attr[1];
	Ndbtuple *t;

	snprint(dest, len, "%s", name);
	attr[0] = "dom";
	t = csipinfo(nil, ipattr(name), name, attr, 1);
	if(t != nil){
		snprint(dest, len, "%s", t->val);
		ndbfree(t);
	}
}

/*
 *  convert b into a big-endian array of bytes, right justified in buf
 */
void
mptobuf(mpint *b, uchar *buf, int len)
{
	int n;

	n = mptobe(b, buf, len, nil);
	assert(n >= 0);
	if(n < len){
		len -= n;
		memmove(buf+len, buf, n);
		memset(buf, 0, len);
	}
}
