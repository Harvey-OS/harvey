#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "bzfs.h"
#include "bzlib.h"

static int
bunzip(int ofd, char *ofile, Biobuf *bin)
{
	int e, n, done, onemore;
	char buf[8192];
	char obuf[8192];
	Biobuf bout;
	bz_stream strm;

	USED(ofile);

	memset(&strm, 0, sizeof strm);
	BZ2_bzDecompressInit(&strm, 0, 0);

	strm.next_in = buf;
	strm.avail_in = 0;
	strm.next_out = obuf;
	strm.avail_out = sizeof obuf;

	done = 0;
	Binit(&bout, ofd, OWRITE);

	/*
	 * onemore is a crummy hack to go 'round the loop
	 * once after we finish, to flush the output buffer.
	 */
	onemore = 1;
	SET(e);
	do {
		if(!done && strm.avail_in < sizeof buf) {
			if(strm.avail_in)
				memmove(buf, strm.next_in, strm.avail_in);
			
			n = Bread(bin, buf+strm.avail_in, sizeof(buf)-strm.avail_in);
			if(n <= 0)
				done = 1;
			else
				strm.avail_in += n;
			strm.next_in = buf;
		}
		if(strm.avail_out < sizeof obuf) {
			Bwrite(&bout, obuf, sizeof(obuf)-strm.avail_out);
			strm.next_out = obuf;
			strm.avail_out = sizeof obuf;
		}

		if(onemore == 0)
			break;
	} while((e=BZ2_bzDecompress(&strm)) == BZ_OK || onemore--);

	if(e != BZ_STREAM_END) {
		fprint(2, "bunzip2: decompress failed\n");
		return 0;
	}

	if(BZ2_bzDecompressEnd(&strm) != BZ_OK) {
		fprint(2, "bunzip2: decompress end failed (can't happen)\n");
		return 0;
	}

	Bterm(&bout);

	return 1;
}

int
bzexpand(int in)
{
	int rv, out, p[2];
	Biobuf bin;

	if(pipe(p) < 0)
		sysfatal("pipe: %r");

	rv = p[0];
	out = p[1];
	switch(rfork(RFPROC|RFFDG|RFNOTEG)){
	case -1:
		sysfatal("fork: %r");
	case 0:
		close(rv);
		break;
	default:
		close(in);
		close(out);
		return rv;
	}

	Binit(&bin, in, OREAD);
	if(bunzip(out, nil, &bin) != 1) {
		fprint(2, "bunzip2 failed\n");
		_exits("bunzip2");
	}
	_exits(0);
	return -1;	/* not reached */
}
