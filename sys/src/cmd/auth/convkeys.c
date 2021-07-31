#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

char	authkey[DESKEYLEN];
int	verb;
int	usepass;

int	convert(char*, char*, int);
int	dofcrypt(int, char*, char*, int);
void	usage(void);

void
main(int argc, char *argv[])
{
	Dir d;
	char *p, *file, key[DESKEYLEN];
	int fd, len;

	ARGBEGIN{
	case 'v':
		verb = 1;
		break;
	case 'p':
		usepass = 1;
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
		getpass(authkey, 0);
	} else
		getauthkey(authkey);
	print("enter password to reencode with\n");
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
	fprint(2, "usage: convkeys keyfile\n");
	exits("usage");
}
