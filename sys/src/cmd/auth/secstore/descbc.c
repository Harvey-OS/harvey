/*
 * encrypt or decrypt file with DES by writing
 *	16byte initialization vector,
 *	DES-CBC(key, random | file),
 *	HMAC_SHA1(md5(key), DES-CBC(random | file))
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mp.h>
#include <libsec.h>
#include <authsrv.h>

extern char* getpassm(char*);

enum{ CHK = 16, BUF = 4096 };

static uchar v2hdr[] = "DES CBC SHA1  1\n";
static Biobuf bin, bout;
static int oldcbc;
static char authkey[8];

void
safewrite(uchar *buf, int n)
{
	if(Bwrite(&bout, buf, n) != n)
		sysfatal("write error");
}

void
saferead(uchar *buf, int n)
{
	if(Bread(&bin, buf, n) != n)
		sysfatal("read error");
}

void
oldCBCencrypt(char *key7, uchar *p, int len)
{
	uchar ivec[8], key[8];
	DESstate s;

	memset(ivec, 0, 8);
	des56to64((uchar*)key7, key);
	setupDESstate(&s, key, ivec);
	desCBCencrypt((uchar*)p, len, &s);
}

void
oldCBCdecrypt(char *key7, uchar *p, int len)
{
	uchar ivec[8], key[8];
	DESstate s;

	memset(ivec, 0, 8);
	des56to64((uchar*)key7, key);
	setupDESstate(&s, key, ivec);
	desCBCdecrypt((uchar*)p, len, &s);
}

void
main(int argc, char **argv)
{
	int encrypt = 0;		/* 0=decrypt, 1=encrypt */
	int n, nkey, pass_stdin = 0, pass_nvram = 0;
	char *pass;
	DESstate des;
	uchar key[sizeof des.key], key2[SHA1dlen];
	uchar buf[BUF+SHA1dlen];	/* assumption: CHK <= SHA1dlen */
	DigestState *dstate;
	Nvrsafe nvr;

	ARGBEGIN{
	case 'e':
		encrypt = 1;
		break;
	case 'i':
		pass_stdin = 1;
		break;
	case 'n':
		pass_nvram = 1;
		break;
	case 'o':
		/*
		 * TODO use nvr.machkey, a des key, converted to 64 bits.
		 * see keyfs.
		 */
		oldcbc = 1;
		break;
	}ARGEND;
	if(argc!=0){
		fprint(2, "usage: %s -d [-ino] <cipher.des >clear.txt\n", argv0);
		fprint(2, "   or: %s -e [-ino] <clear.txt >cipher.des\n", argv0);
		exits("usage");
	}
	Binit(&bin, 0, OREAD);
	Binit(&bout, 1, OWRITE);

	if(pass_stdin){
		n = readn(3, buf, sizeof buf - 1);
		if(n < 1)
			sysfatal("usage: echo password |[3=1] auth/descbc -i ...");
		buf[n] = 0;
		while(buf[n-1] == '\n')
			buf[--n] = 0;
	}else if(pass_nvram){
		if(readnvram(&nvr, 0) < 0)
			exits("readnvram: %r");
		strecpy((char*)buf, (char*)buf+sizeof buf, (char*)nvr.config);
		n = strlen((char*)buf);
	}else{
		pass = getpassm("descbc key:");
		n = strlen(pass);
		if(n >= BUF)
			exits("key too long");
		strcpy((char*)buf, pass);
		memset(pass, 0, n);
		free(pass);
	}
	if(n <= 0)
		sysfatal("no key");
	dstate = sha1((uchar*)"descbc file", 11, nil, nil);
	sha1(buf, n, key2, dstate);
	memcpy(key, key2, 16);
	nkey = 16;
	/* so even if HMAC_SHA1 is broken, encryption key is protected */
	md5(key, nkey, key2, 0);

	if(encrypt){
//		safewrite(v2hdr, DESbsize);
		/* CBC is semantically secure if IV is unpredictable. */
		genrandom(buf, 2*DESbsize);
		/* use first DESbsize bytes as IV */
		setupDESstate(&des, key, buf);
		/* use second DESbsize bytes as initial plaintext */
		desCBCencrypt(buf+DESbsize, DESbsize, &des);
		safewrite(buf, 2*DESbsize);
		dstate = hmac_sha1(buf+DESbsize, DESbsize, key2, MD5dlen, 0, 0);
		for(;;){
			n = Bread(&bin, buf, BUF);
			if(n < 0)
				sysfatal("read error");
			desCBCencrypt(buf, n, &des);
			safewrite(buf, n);
			dstate = hmac_sha1(buf, n, key2, MD5dlen, 0, dstate);
			if(n < BUF)
				break; /* EOF */
		}
		hmac_sha1(0, 0, key2, MD5dlen, buf, dstate);
		safewrite(buf, SHA1dlen);
	}else{						/* decrypt */
		saferead(buf, DESbsize);
		if(memcmp(buf, v2hdr, DESbsize) == 0){
			/* read IV and random initial plaintext */
			saferead(buf, 2*DESbsize);
			setupDESstate(&des, key, buf);
			dstate = hmac_sha1(buf+DESbsize, DESbsize, key2,
				MD5dlen, 0, 0);
			if (oldcbc)
				/* TODO generate authkey */
				oldCBCdecrypt(authkey, buf+DESbsize, DESbsize);
			else
				desCBCdecrypt(buf+DESbsize, DESbsize, &des);
			saferead(buf, SHA1dlen);
			while((n = Bread(&bin, buf+SHA1dlen, BUF)) > 0){
				dstate = hmac_sha1(buf, n, key2, MD5dlen, 0,
					dstate);
				desCBCdecrypt(buf, n, &des);
				safewrite(buf, n);
				/* these bytes are not yet decrypted */
				memmove(buf, buf+n, SHA1dlen);
			}
			hmac_sha1(0, 0, key2, MD5dlen, buf+SHA1dlen, dstate);
			if(memcmp(buf, buf+SHA1dlen, SHA1dlen) != 0)
				sysfatal("decrypted file failed to authenticate");
		}else{
			/*
			 * no header, just DES-encrypted data
			 */
			setupDESstate(&des, key, buf);
			saferead(buf, CHK);
			desCBCdecrypt(buf, CHK, &des);
			while((n = Bread(&bin, buf+CHK, BUF)) > 0){
				desCBCdecrypt(buf+CHK, n, &des);
				safewrite(buf, n);
				memmove(buf, buf+n, CHK);
			}
		}
	}
	exits(nil);
}
