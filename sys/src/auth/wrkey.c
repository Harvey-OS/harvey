#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

#define	NVPASSWD	1024+900
void	usage(void);

void main(int argc, char *argv[]){
	Nvrsafe safe;
	char *p, key[DESKEYLEN];
	int fd, iskey;

	iskey = 0;
	ARGBEGIN{
	case 'k':
		p = ARGF();
		if(strlen(p) != DESKEYLEN)
			error("bad key: must be seven characters long\n");
		memcpy(key, p, DESKEYLEN);
		iskey = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 0)
		usage();
	USED(argv);

	if(!iskey)
		getpass(key, 0);

	fd = open("#r/nvram", ORDWR);
	if(fd < 0)
		error("can't open nvram");
	if(seek(fd, NVPASSWD, 0) < 0
	|| read(fd, &safe, sizeof safe) != sizeof safe)
		error("can't read nvram");
	memmove(safe.machkey, key, DESKEYLEN);
	safe.machsum = nvcsum(safe.machkey, DESKEYLEN);
	if(seek(fd, NVPASSWD, 0) < 0
	|| write(fd, &safe, sizeof safe) != sizeof safe)
		error("write nvram");
	exits(0);
}

void
usage(void)
{
	fprint(2, "usage: wrkey [-k key]\n");
	exits("usage");
}
