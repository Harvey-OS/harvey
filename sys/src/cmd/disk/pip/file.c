#include	"all.h"

enum
{
	Nblock		= 12,
};

void
fload(void)
{
	int type, totrack, f;
	long bs, n, m;
	uchar *buf;

	type = gettype("type [cdda/cdrom]");
	switch(type) {
	default:
		print("bad type\n");
		return;
	case Tcdrom:
		bs = Bcdrom;
		break;
	case Tcdda:
		bs = Bcdda;
		break;
	}

	if(eol()) {
		print("file name: ");
	}
	getword();
	f = open(word, OREAD);
	if(f < 0) {
		print("cant open %s: %r\n", word);
		return;
	}
	totrack = gettrack("disk track [number]");

	buf = dbufalloc(Nblock, bs);
	if(buf == 0)
		goto out;
	for(;;) {
		n = read(f, buf, Nblock*bs);
		if(n != Nblock*bs)
			break;
		if(dwrite(buf, Nblock))
			goto out;
	}
	if(n != 0) {
		if(n < 0) {
			print("file read error: %r\n");
			goto out;
		}
		m = n % bs;
		if(m)
			memset(buf+n, 0, bs-m);
		n = (n + bs - 1) / bs;
		if(dwrite(buf, n))
			goto out;
	}
	dcommit(totrack, type);
out:
	close(f);
}
