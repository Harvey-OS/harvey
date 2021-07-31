#include <u.h>
#include <libc.h>
#include "ip.h"

int
eipconv(void *v, Fconv *f)
{
	static char buf[64];
	static char *efmt = "%.2lux%.2lux%.2lux%.2lux%.2lux%.2lux";
	static char *ifmt = "%d.%d.%d.%d";
	uchar *p;

	p = *((uchar**)v);
	switch(f->chr) {
	case 'E':		/* Ethernet address */
		sprint(buf, efmt, p[0], p[1], p[2], p[3], p[4], p[5]);
		break;
	case 'I':		/* Ip address */
		sprint(buf, ifmt, p[0], p[1], p[2], p[3]);
		break;
	default:
		strcpy(buf, "BAD");
	}
	strconv(buf, f);
	return sizeof(uchar*);
}
