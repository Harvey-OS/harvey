#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include <libsec.h>
#include "wrap.h"

DigestState *md5s;

void
main(void)
{
	uchar digest[MD5dlen], digest0[MD5dlen];

	fmtinstall('M', md5conv);

	md5s = md5(nil, 0, nil, nil);
	md5((uchar*)"hello world", 11, nil, md5s);
	md5(nil, 0, digest, md5s);

	md5s = md5(nil, 0, nil, nil);
	md5((uchar*)"hello world", 11, nil, md5s);
	md5(nil, 0, digest0, md5s);

	print("%M %M\n", digest, digest0);
}
