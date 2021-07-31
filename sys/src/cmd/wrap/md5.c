#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include <libsec.h>
#include "wrap.h"

int
md5file(char *file, uchar *digest)
{
	Biobuf *b;
	DigestState *s;
	uchar buf[8192];
	int n;

	if((b = Bopen(file, OREAD)) == nil)
		return -1;

	s = md5(nil, 0, nil, nil);
	while((n = Bread(b, buf, sizeof buf)) > 0)
		md5(buf, n, nil, s);
	md5(nil, 0, digest, s);
	Bterm(b);
	return 0;
}

int
Bmd5sum(Biobuf *b, uchar *digest, vlong len)
{
	DigestState *s;
	uchar buf[8192];
	int n;

	s = md5(nil, 0, nil, nil);
	while(len > 0) {
		if(len > sizeof buf)
			n = sizeof(buf);
		else
			n = len;
		if((n = Bread(b, buf, n)) < 0)
			return -1;
		if(n == 0) {
			werrstr("premature end of file");
			return -1;
		}
		md5(buf, n, nil, s);
		len -= n;
	}
	md5(nil, 0, digest, s);
	return 0;
}

int
md5conv(va_list *va, Fconv *fp)
{
	char buf[MD5dlen*2+1];
	uchar *p;
	int i;

	p = va_arg(*va, uchar*);
	for(i=0; i<MD5dlen; i++)
		sprint(buf+2*i, "%.2ux", p[i]);
	strconv(buf, fp);
	return 0;
}

