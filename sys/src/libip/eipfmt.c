/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <ip.h>

enum
{
	Isprefix= 16,
};

unsigned char prefixvals[256] =
{
[0x00]	= 0 | Isprefix,
[0x80]	= 1 | Isprefix,
[0xC0]	= 2 | Isprefix,
[0xE0]	= 3 | Isprefix,
[0xF0]	= 4 | Isprefix,
[0xF8]	= 5 | Isprefix,
[0xFC]	= 6 | Isprefix,
[0xFE]	= 7 | Isprefix,
[0xFF]	= 8 | Isprefix,
};

int
eipfmt(Fmt *f)
{
	char buf[5*8];
	static char *efmt = "%.2ux%.2ux%.2ux%.2ux%.2ux%.2ux";
	static char *ifmt = "%d.%d.%d.%d";
	uint8_t *p, ip[16];
	uint32_t *lp;
	uint16_t s;
	int i, j, n, eln, eli;

	switch(f->r) {
	case 'E':		/* Ethernet address */
		p = va_arg(f->args, uint8_t*);
		snprint(buf, sizeof buf, efmt, p[0], p[1], p[2], p[3], p[4], p[5]);
		return fmtstrcpy(f, buf);

	case 'I':		/* Ip address */
		p = va_arg(f->args, uint8_t*);
common:
		if(memcmp(p, v4prefix, 12) == 0){
			snprint(buf, sizeof buf, ifmt, p[12], p[13], p[14], p[15]);
			return fmtstrcpy(f, buf);
		}

		/* find longest elision */
		eln = eli = -1;
		for(i = 0; i < 16; i += 2){
			for(j = i; j < 16; j += 2)
				if(p[j] != 0 || p[j+1] != 0)
					break;
			if(j > i && j - i > eln){
				eli = i;
				eln = j - i;
			}
		}

		/* print with possible elision */
		n = 0;
		for(i = 0; i < 16; i += 2){
			if(i == eli){
				n += sprint(buf+n, "::");
				i += eln;
				if(i >= 16)
					break;
			} else if(i != 0)
				n += sprint(buf+n, ":");
			s = (p[i]<<8) + p[i+1];
			n += sprint(buf+n, "%ux", s);
		}
		return fmtstrcpy(f, buf);

	case 'i':		/* v6 address as 4 longs */
		lp = va_arg(f->args, uint32_t*);
		for(i = 0; i < 4; i++)
			hnputl(ip+4*i, *lp++);
		p = ip;
		goto common;

	case 'V':		/* v4 ip address */
		p = va_arg(f->args, uint8_t*);
		snprint(buf, sizeof buf, ifmt, p[0], p[1], p[2], p[3]);
		return fmtstrcpy(f, buf);

	case 'M':		/* ip mask */
		p = va_arg(f->args, uint8_t*);

		/* look for a prefix mask */
		for(i = 0; i < 16; i++)
			if(p[i] != 0xff)
				break;
		if(i < 16){
			if((prefixvals[p[i]] & Isprefix) == 0)
				goto common;
			for(j = i+1; j < 16; j++)
				if(p[j] != 0)
					goto common;
			n = 8*i + (prefixvals[p[i]] & ~Isprefix);
		} else
			n = 8*16;

		/* got one, use /xx format */
		snprint(buf, sizeof buf, "/%d", n);
		return fmtstrcpy(f, buf);
	}
	return fmtstrcpy(f, "(eipfmt)");
}
