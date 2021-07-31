#include <u.h>
#include <libc.h>
#include "mips.out.h"

#define	u_char	uchar
#define	u_short	ushort
#define	int	long
#include "dvh.h"
#undef	int

#define	HLEN	(sizeof(struct mipshdr))

long	vhdrbuf[512/sizeof(long)];

void
main(int argc, char **argv)
{
	struct volume_header *vhp = (struct volume_header *)vhdrbuf;
	struct mipshdr *mp;
	int fd = 0;
	long n, len;
	uchar *buf;
	long *p;

	ARGBEGIN{
	default:
		fprint(2, "usage: %s [file]\n", argv0);
		exits("usage");
	}ARGEND

	if(argc > 0){
		fd = open(argv[0], OREAD);
		if(fd < 0){
			perror(argv[0]);
			exits("open");
		}
	}
	buf = malloc(HLEN);
	if(buf == 0){
		fprint(2, "malloc(%d) failed\n", HLEN);
		exits("malloc");
	}
	n = read(fd, buf, HLEN);
	if(n != HLEN){
		fprint(2, "read hdr returned %d\n", n);
		if(n < 0)
			perror("");
		exits("hdr");
	}
	mp = (struct mipshdr *)buf;
	len = HLEN + mp->tsize + mp->dsize;
	if(len <= HLEN || len > 0x100000){
		fprint(2, "weird len %d\n", len);
		exits("len");
	}
	mp->f_symptr = 0;
	mp->f_nsyms = 0;
	mp->f_flags |= F_LSYMS;
	buf = realloc(buf, len);
	if(buf == 0){
		fprint(2, "realloc(%d) failed\n", len);
		exits("realloc");
	}
	n = read(fd, buf+HLEN, len-HLEN);
	if(n != len-HLEN){
		fprint(2, "read %d returned %d\n", len-HLEN, n);
		if(n < 0)
			perror("");
		exits("read");
	}

	vhp->vh_magic = VHMAGIC;
	strcpy(vhp->vh_bootfile, "b");
	vhp->vh_dp.dp_cyls = 1;
	vhp->vh_dp.dp_trks0 = 1;
	vhp->vh_dp.dp_secs = 64;
	vhp->vh_dp.dp_secbytes = 512;

	strcpy(vhp->vh_vd[0].vd_name, "b");
	vhp->vh_vd[0].vd_lbn = 1;
	vhp->vh_vd[0].vd_nbytes = len;

	vhp->vh_pt[0].pt_nblks = 64;
	vhp->vh_pt[0].pt_firstlbn = 0;
	vhp->vh_pt[0].pt_type = PTYPE_VOLHDR;

	n = 0;
	for(p=vhdrbuf; p<&vhdrbuf[sizeof vhdrbuf/sizeof(long)]; p++)
		n += *p;
	vhp->vh_csum = -n;

	write(1, vhdrbuf, sizeof vhdrbuf);
	n = write(1, buf, len);
	if(n != len){
		fprint(2, "write %d returned %d\n", len, n);
		if(n < 0)
			perror("");
		exits("write");
	}
	exits(0);
}
