#include <u.h>
#include <libc.h>
#include "ip.h"

int
eipconv(void *v, int f1, int f2, int f3, int ch)
{
	static char buf[64];
	static char *efmt = "%.2lux%.2lux%.2lux%.2lux%.2lux%.2lux";
	static char *ifmt = "%d.%d.%d.%d";
	uchar *p;

	p = *((uchar**)v);
	switch(ch) {
	case 'E':		/* Ethernet address */
		sprint(buf, efmt, p[0], p[1], p[2], p[3], p[4], p[5]);
		break;
	case 'I':		/* Ip address */
		sprint(buf, ifmt, p[0], p[1], p[2], p[3]);
		break;
	default:
		strcpy(buf, "BAD");
	}
	strconv(buf, f1, f2, f3);
	return sizeof(uchar*);
}
