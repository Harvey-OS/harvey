#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <authsrv.h>
#include <mp.h>
#include <libsec.h>
#include <bio.h>
#include "authcmdlib.h"

char	authkey[DESKEYLEN];
int	verb;
int	usepass;

int	convert(char*, char*, int);
int	dofcrypt(int, char*, char*, int);
void	usage(void);

void
main(int argc, char *argv[])
{
	Dir *d;
	char *p, *file, key[DESKEYLEN];
	int fd, len;

	ARGBEGIN{
	case 'p':
		usepass = 1;
		break;
	case 'v':
		verb = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();
	file = argv[0];

	/* get original key */
	if(usepass){
		print("enter password file is encoded with\n");
		getpass(authkey, nil, 0, 1);
	} else
		getauthkey(authkey);
	if(!verb){
		print("enter password to reencode with\n");
		getpass(key, nil, 0, 1);
	}

	fd = open(file, ORDWR);
	if(fd < 0)
		error("can't open %s: %r\n", file);
	d = dirfstat(fd);
	if(d == nil)
		error("can't stat %s: %r\n", file);
	len = d->length;
	p = malloc(len);
	if(!p)
		error("out of memory");
	if(read(fd, p, len) != len)
		error("can't read key file: %r\n");
	len = convert(p, key, len);
	if(verb)
		exits(0);
	if(pwrite(fd, p, len, 0) != len)
		error("can't write key file: %r\n");
	close(fd);
	exits(0);
}

void
randombytes(uchar *p, int len)
{
	int i, fd;

	fd = open("/dev/random", OREAD);
	if(fd < 0){
		fprint(2, "convkeys: can't open /dev/random, using rand()\n");
		srand(time(0));
		for(i = 0; i < len; i++)
			p[i] = rand();
		return;
	}
	read(fd, p, len);
	close(fd);
}

void
oldCBCencrypt(char *key7, char *p, int len)
{
	uchar ivec[8];
	uchar key[8];
	DESstate s;

	memset(ivec, 0, 8);
	des56to64((uchar*)key7, key);
	setupDESstate(&s, key, ivec);
	desCBCencrypt((uchar*)p, len, &s);
}

void
oldCBCdecrypt(char *key7, char *p, int len)
{
	uchar ivec[8];
	uchar key[8];
	DESstate s;

	memset(ivec, 0, 8);
	des56to64((uchar*)key7, key);
	setupDESstate(&s, key, ivec);
	desCBCdecrypt((uchar*)p, len, &s);

}

static int
badname(char *s)
{
	int n;
	Rune r;

	for (; *s != '\0'; s += n) {
		n = chartorune(&r, s);
		if (n == 1 && r == Runeerror)
			return 1;
	}
	return 0;
}

int
convert(char *p, char *key, int len)
{
	int i;

	len -= KEYDBOFF;
	if(len % KEYDBLEN){
		fprint(2, "convkeys: file odd length; not converting %d bytes\n",
			len % KEYDBLEN);
		len -= len % KEYDBLEN;
	}
	len += KEYDBOFF;
	oldCBCdecrypt(authkey, p, len);
	for(i = KEYDBOFF; i < len; i += KEYDBLEN)
		if (badname(&p[i])) {
			print("bad name %.30s... - aborting\n", &p[i]);
			return 0;
		}
	if(verb)
		for(i = KEYDBOFF; i < len; i += KEYDBLEN)
			print("%s\n", &p[i]);

	randombytes((uchar*)p, 8);
	oldCBCencrypt(key, p, len);
	return len;
}

void
usage(void)
{
	fprint(2, "usage: convkeys keyfile\n");
	exits("usage");
}
