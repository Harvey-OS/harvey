/* encrypt file by writing 16byte initialization vector,
	then AES CBC encrypted bytes of (file, 16 bytes of X) */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mp.h>
#include <libsec.h>

enum{ CHK = 16, BUF = 4096 };

int
decode16(uchar *dest, int ndest, char *src, int nsrc)
{
	int c, v, w = 0;
	uchar *start = dest;

	if(nsrc%2 != 0 || ndest < nsrc/2)
		return 0;
	while(--nsrc >= 0){
		c = *src++;
		if('0' <= c && c <= '9')
			v = c - '0';
		else if('a' <= c && c <= 'z')
			v = c - 'a' + 10;
		else if('A' <= c && c <= 'Z')
			v = c - 'A' + 10;
		else
			v = 0;
		w = w<<4 + v;
		if(nsrc&1 == 0){
			*dest++ = w;
			w = 0;
		}
	}
	return dest - start;
}

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
main(int argc, char **argv)
{
	int encrypt = 0;  /* 0=decrypt, 1=encrypt */
	int i, n, nkey, done;
	char *hex;
	uchar key[AESmaxkey], buf[CHK+BUF];
	AESstate aes;

	if(argc!=2 || argv[1][0]!='-'){
		fprint(2,"usage: HEX=key %s -d < cipher.aes > clear.txt\n", argv[0]);
		fprint(2,"   or: HEX=key %s -e < clear.txt > cipher.aes\n", argv[0]);
		exits("usage");
	}
	if(argv[1][1] == 'e')
		encrypt = 1;
	Binit(&bin, 0, OREAD);
	Binit(&bout, 1, OWRITE);

	hex = getenv("HEX");
	nkey = strlen(hex);
	if((nkey&1) || nkey>2*AESmaxkey){
		fprint(2,"key should be 32 hex digits\n");
		exits("key");
	}
	nkey = decode16(key, sizeof key, hex, nkey);

	if(encrypt){
		srand(time(0));  /* doesn't need to be unpredictable */
		for(i=0; i<AESbsize; i++)
			buf[i] = 0xff & rand();
		Bwrite(&bout, buf, AESbsize);
		setupAESstate(&aes, key, nkey, buf);
		for(done = 0; !done; ){
			n = Bread(&bin, buf, BUF);
			if(n < 0){
				fprint(2,"read error\n");
				exits("read error");
			}
			if(n < BUF){ /* EOF on input; append XX... */
				memset(buf+n, 'X', CHK);
				n += CHK;
				done = 1;
			}
			aesCBCencrypt(buf, n, &aes);
			safewrite(buf, n);
		}
	}else{
		Bread(&bin, buf, AESbsize);
		setupAESstate(&aes, key, nkey, buf);
		n = Bread(&bin, buf, CHK);
		assert(n == CHK);
		aesCBCdecrypt(buf, n, &aes);
		while((n = Bread(&bin, buf+CHK, BUF)) > 0){
			aesCBCdecrypt(buf+CHK, n, &aes);
			safewrite(buf, n);
			memmove(buf, buf+n, CHK);
		}
		if(memcmp(buf, "XXXXXXXXXXXXXXXX", CHK) != 0){
			fprint(2,"decrypted file failed to authenticate!\n");
			exits("decrypted file failed to authenticate");
		}
	}
	exits("");
}
