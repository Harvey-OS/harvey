#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

char	authkey[DESKEYLEN];
int	verb;
int	cryptfd;

int	convert(char*, char*, int);
int	dofcrypt(int, char*, char*, int);
void	usage(void);

void
main(int argc, char *argv[])
{
	Dir d;
	char *p, *file, key[DESKEYLEN];
	int fd, nvkey, havekey, len;

	havekey = 0;
	nvkey = 1;
	ARGBEGIN{
	case 'd':
		nvkey = 0;
		cryptfd = open("/dev/crypt", ORDWR);
		if(cryptfd < 0)
			error("can't open /dev/crypt: %r\n");
		break;
	case 'k':
		p = ARGF();
		if(strlen(p) != DESKEYLEN)
			error("bad key: must be seven characters long\n");
		memcpy(key, p, DESKEYLEN);
		havekey = 0;
		break;
	case 'v':
		verb = 1;
		havekey = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();
	file = argv[0];

	if(nvkey)
		getauthkey(authkey);
	if(!havekey)
		getpass(key, 0);

	fd = open(file, ORDWR);
	if(fd < 0)
		error("can't open %s: %r\n", file);
	if(dirfstat(fd, &d) < 0)
		error("can't stat %s: %r\n", file);
	len = d.length;
	p = malloc(len);
	if(!p)
		error("out of memory");
	if(read(fd, p, len) != len)
		error("can't read key file: %r\n");
	len = convert(p, key, len);
	if(verb)
		exits(0);
	if(seek(fd, 0, 0) < 0 || write(fd, p, len) != len)
		error("can't write key file: %r\n");
	close(fd);
	exits(0);
}

int
convert(char *p, char *key, int len)
{
	int i;

	if(len % KEYDBLEN){
		fprint(2, "file odd length; not converting %d bytes\n", len % KEYDBLEN);
		len -= len % KEYDBLEN;
	}
	for(i = 0; i < len; i += KEYDBLEN){
		if(cryptfd < 0)
			decrypt(authkey, &p[i], KEYDBLEN);
		else
			dofcrypt(cryptfd, "D", &p[i], KEYDBLEN);
		if(verb)
			print("%s\n", &p[i]);
		else
			encrypt(key, &p[i], KEYDBLEN);
	}
	return len;
}

/*
 * encrypt/decrypt data using a crypt device
 */
int
dofcrypt(int fd, char *how, char *s, int n)
{
	
	if(write(fd, how, 1) < 1)
		return -1;
	seek(fd, 0, 0);
	if(write(fd, s, n) < n)
		return -1;
	seek(fd, 0, 0);
	if(read(fd, s, n) < n)
		return -1;
	return n;
}

void
usage(void)
{
	fprint(2, "usage: convkeys [-d] [-k key] keyfile\n");
	exits("usage");
}
