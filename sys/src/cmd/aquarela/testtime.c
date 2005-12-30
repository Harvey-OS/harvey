#include "headers.h"

void
threadmain(int argc, char **argv)
{
	ulong now, now2;
	vlong nttime;
	if (argc > 1) {
		nttime = strtoull(argv[1], 0, 0);
		now2 = smbtime2plan9time(nttime);
		print("%ld %s", now2, ctime(now2));
	}
	else {
		now = 1032615845;
		nttime = smbplan9time2time(now);
		print("0x%.llux\n", nttime);
		now2 = smbtime2plan9time(nttime);
		print("now %ld %s", now, ctime(now));
		print("now2 %ld %s", now2, ctime(now2));
	}
}


