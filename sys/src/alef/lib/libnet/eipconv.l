#include <alef.h>
#include <bio.h>
#include <net.h>

intern byte *efmt = "%.2ux%.2ux%.2ux%.2ux%.2ux%.2ux";
intern byte *ifmt = "%d.%d.%d.%d";

int
eipconv(Printspec *ps)
{
	byte *p;
	byte buf[64], ip[4];

	p = *((byte**)ps->o);
	switch(ps->chr) {
	case 'E':		/* Ethernet address */
		sprint(buf, efmt, p[0], p[1], p[2], p[3], p[4], p[5]);
		break;
	case 'I':		/* Ip address */
		sprint(buf, ifmt, p[0], p[1], p[2], p[3]);
		break;
	case 'i':
		hnputl(ip, *(uint*)ps->o);
		sprint(buf, ifmt, ip[0], ip[1], ip[2], ip[3]);
		break;
	default:
		strcpy(buf, "*eipconv*");
	}
	strconv(ps, buf);
	return sizeof(byte*);
}
