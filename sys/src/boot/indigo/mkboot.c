#include <u.h>
#include <libc.h>
#include "dvh.h"

Vhdr	vhdr;

uchar	buf[8192];

void
usage(void)
{
	fprint(2, "usage: %s file\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	Dir dir;
	int fd;
	long k, n, len;
	long *p;

	ARGBEGIN{
	default:
		usage();
		break;
	}ARGEND

	if(argc != 1)
		usage();
	fd = open(argv[0], OREAD);
	if(fd < 0){
		fprint(2, "%s: can't open %s: %r\n", argv0, argv[0]);
		exits("open");
	}
	if(dirfstat(fd, &dir) < 0){
		fprint(2, "%s: can't fstat %s: %r\n", argv0, argv[0]);
		exits("fstat");
	}
	len = dir.length;

	vhdr.magic = VMAGIC;
	strcpy(vhdr.boot, "b");
	vhdr.cyls = 1;
	vhdr.trks0 = 1;
	vhdr.sec2trk = 2048;
	vhdr.bytes2sec = 512;

	strcpy(vhdr.vd[0].name, "b");
	vhdr.vd[0].lbn = 1;
	vhdr.vd[0].nbytes = len;

	vhdr.pt[0].nblks = 2048;
	vhdr.pt[0].firstlbn = 0;
	vhdr.pt[0].type = VOLHDR;

	vhdr.pt[10].nblks = 2048;
	vhdr.pt[10].firstlbn = 0;
	vhdr.pt[10].type = VOLUME;

	n = 0;
	for(p=vhdr.buf; p<&vhdr.buf[nelem(vhdr.buf)]; p++)
		n += *p;
	vhdr.csum = -n;

	write(1, &vhdr, sizeof vhdr);
	for(; len>0; len-=k){
		k = len;
		if(k > sizeof buf)
			k = sizeof buf;
		n = read(fd, buf, k);
		if(n != k){
			fprint(2, "read %d returned %d\n", k, n);
			if(n < 0)
				perror("");
			exits("read");
		}
		n = write(1, buf, k);
		if(n != k){
			fprint(2, "write %d returned %d\n", k, n);
			if(n < 0)
				perror("");
			exits("write");
		}
	}
	exits(0);
}
