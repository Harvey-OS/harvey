/* encrypt file by writing
	v2hdr,
	16byte initialization vector,
	AES-CBC(key, random | file),
    HMAC_SHA1(md5(key), AES-CBC(random | file))

With CBC, if the first plaintext block is 0, the first ciphertext block is E(IV).
Using the overflow technique adopted for compatibility with cryptolib makes the
last cipertext block decryptable.  Hence the random prefix to file.
*/
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mp.h>
#include <libsec.h>

int getpasswd(char*, char*, int);

enum{ CHK = 16, BUF = 4096 };

uchar v2hdr[AESbsize+1] = "AES CBC SHA1  2\n";
Biobuf bin;
Biobuf bout;

void
safewrite(uchar *buf, int n)
{
	int i = Bwrite(&bout, buf, n);

	if(i == n)
		return;
	fprint(2, "write error\n");
	exits("write error");
}

void
saferead(uchar *buf, int n)
{
	int i = Bread(&bin, buf, n);

	if(i == n)
		return;
	fprint(2, "read error\n");
	exits("read error");
}

void
main(int argc, char **argv)
{
	int encrypt = 0;  /* 0=decrypt, 1=encrypt */
	int n, nkey;
	char *hex;
	uchar key[AESmaxkey], key2[SHA1dlen];
	uchar buf[BUF+SHA1dlen];    /* assumption: CHK <= SHA1dlen */
	AESstate aes;
	DigestState *dstate;

	if(argc!=2 || argv[1][0]!='-'){
		fprint(2,"usage: %s -d < cipher.aes > clear.txt\n", argv[0]);
		fprint(2,"   or: %s -e < clear.txt > cipher.aes\n", argv[0]);
		exits("usage");
	}
	if(argv[1][1] == 'e')
		encrypt = 1;
	Binit(&bin, 0, OREAD);
	Binit(&bout, 1, OWRITE);

	hex = getenv("HEX");
	if(hex != nil){
		nkey = strlen(hex);
		if((nkey&1) || nkey>2*AESmaxkey){
			fprint(2,"key should be 32 hex digits\n");
			exits("key");
		}
		nkey = dec16(key, sizeof key, hex, nkey);
	}else{
		n = getpasswd("password:", (char*)buf, sizeof buf);
		if(n < 0){
			fprint(2,"no key\n");
			exits("key");
		}
		dstate = sha1((uchar*)"aescbc file", 11, nil, nil);
		sha1(buf, n, key2, dstate);
		memcpy(key, key2, 16);
		nkey = 16;
	}
	md5(key, nkey, key2, 0);  /* so even if HMAC_SHA1 is broken, encryption key is protected */

	if(encrypt){
		safewrite(v2hdr, AESbsize);
		genrandom(buf,2*AESbsize); /* CBC is semantically secure if IV is unpredictable. */
		setupAESstate(&aes, key, nkey, buf);  /* use first AESbsize bytes as IV */
		aesCBCencrypt(buf+AESbsize, AESbsize, &aes);  /* use second AESbsize bytes as initial plaintext */
		safewrite(buf, 2*AESbsize);
		dstate = hmac_sha1(buf+AESbsize, AESbsize, key2, MD5dlen, 0, 0);
		while(1){
			n = Bread(&bin, buf, BUF);
			if(n < 0){
				fprint(2,"read error\n");
				exits("read error");
			}
			aesCBCencrypt(buf, n, &aes);
			safewrite(buf, n);
			dstate = hmac_sha1(buf, n, key2, MD5dlen, 0, dstate);
			if(n < BUF)
				break; /* EOF */
		}
		hmac_sha1(0, 0, key2, MD5dlen, buf, dstate);
		safewrite(buf, SHA1dlen);
	}else{ /* decrypt */
		saferead(buf, AESbsize);
		if(memcmp(buf, v2hdr, AESbsize) == 0){
			saferead(buf, 2*AESbsize);  /* read IV and random initial plaintext */
			setupAESstate(&aes, key, nkey, buf);
			dstate = hmac_sha1(buf+AESbsize, AESbsize, key2, MD5dlen, 0, 0);
			aesCBCdecrypt(buf+AESbsize, AESbsize, &aes);
			saferead(buf, SHA1dlen);
			while((n = Bread(&bin, buf+SHA1dlen, BUF)) > 0){
				dstate = hmac_sha1(buf, n, key2, MD5dlen, 0, dstate);
				aesCBCdecrypt(buf, n, &aes);
				safewrite(buf, n);
				memmove(buf, buf+n, SHA1dlen);  /* these bytes are not yet decrypted */
			}
			hmac_sha1(0, 0, key2, MD5dlen, buf+SHA1dlen, dstate);
			if(memcmp(buf, buf+SHA1dlen, SHA1dlen) != 0){
				fprint(2,"decrypted file failed to authenticate!\n");
				exits("decrypted file failed to authenticate!");
			}
		}else{ /* compatibility with past mistake */
			// if file was encrypted with bad aescbc use this:
			//         memset(key, 0, AESmaxkey);
			//    else assume we're decrypting secstore files
			setupAESstate(&aes, key, AESbsize, buf);
			saferead(buf, CHK);
			aesCBCdecrypt(buf, CHK, &aes);
			while((n = Bread(&bin, buf+CHK, BUF)) > 0){
				aesCBCdecrypt(buf+CHK, n, &aes);
				safewrite(buf, n);
				memmove(buf, buf+n, CHK);
			}
			if(memcmp(buf, "XXXXXXXXXXXXXXXX", CHK) != 0){
				fprint(2,"decrypted file failed to authenticate\n");
				exits("decrypted file failed to authenticate");
			}
		}
	}
	exits("");
}
