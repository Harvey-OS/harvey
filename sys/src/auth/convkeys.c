#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

char authkey[DESKEYLEN];
int	verb;

int	convert(char*, char*, int);
void	usage(void);

void
main(int argc, char *argv[])
{
	Dir d;
	char *p, *file, key[DESKEYLEN];
	int fd, iskey, len;

	getauthkey(authkey);
	iskey = 0;
	ARGBEGIN{
	case 'k':
		p = ARGF();
		if(strlen(p) != DESKEYLEN)
			error("bad key: must be seven characters long\n");
		memcpy(key, p, DESKEYLEN);
		iskey = 1;
		break;
	case 'v':
		verb = 1;
		break;
	default:
		usage();
	}ARGEND
	argv0 = "convkeys";

	if(argc != 1)
		usage();
	file = argv[0];
	if(!iskey)
		getpass(key, 0);

	fd = open(file, ORDWR);
	if(fd < 0)
		error("can't open %s", file);
	if(dirfstat(fd, &d) < 0)
		error("can't stat %s\n", file);
	len = d.length;
	p = malloc(len);
	if(!p)
		error("out of memory");
	if(read(fd, p, len) != len)
		error("can't read key file\n");
	len = convert(p, key, len);
	if(verb)
		exits(0);
	if(seek(fd, 0, 0) < 0 || write(fd, p, len) != len)
		error("can't write key file\n");
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
		decrypt(authkey, &p[i], KEYDBLEN);
		if(verb)
			print("%s\n", &p[i]);
		else
			encrypt(key, &p[i], KEYDBLEN);
	}
	return len;
}

void
usage(void)
{
	fprint(2, "usage: convkeys [-k key] keyfile\n");
	exits("usage");
}
