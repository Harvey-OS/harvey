#include <u.h>
#include <libc.h>

#include <ureg.h>

static unsigned char buf[0xFF01];

void
main(int argc, char *argv[])
{
	struct Ureg u;
	int fd, rreg, rmem, len;

	ARGBEGIN {
	} ARGEND;

	if(argc == 0){
		fprint(2, "usage:\t%s file.com\n", argv0);
		exits("usage");
	}
	if((fd = open(*argv, OREAD)) < 0)
		sysfatal("open: %r");

	if((rreg = open("/dev/realmode", OWRITE)) < 0)
		sysfatal("open realmode: %r");
	if((rmem = open("/dev/realmodemem", OWRITE)) < 0)
		sysfatal("open realmodemem: %r");
	if((len = readn(fd, buf, sizeof buf)) < 2)
		sysfatal("file too small");

	memset(&u, 0, sizeof u);
	u.cs = 0x1000;
	u.ss = 0x0000;
	u.sp = 0xfffe;
	u.ip = 0x0100;

	seek(rmem, (u.cs<<4) + u.ip, 0);
	if(write(rmem, buf, len) != len)
		sysfatal("write mem: %r");

	if(write(rreg, &u, sizeof u) != sizeof u)
		sysfatal("write reg: %r");
}
