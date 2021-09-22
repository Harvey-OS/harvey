#include <u.h>
#include <libc.h>
#include <bio.h>
#include "flate.h"

void
main(int argc, char *argv[])
{
	Biobuf *in, *out, *index;
	int i, n;
	char name[64];
	char *file, *arg;
	int offset, slarg, count;
	int dopage;
	char lpcmd[64];
	struct
	{
		char	name[24];
		uchar	xxx[16];
	} dir;

	file = "/lib/aviation/charts.bin";
	arg = "kmmu";
	sprint(lpcmd, "lp -H");
	dopage = 0;

	ARGBEGIN {
	case 'd':
		sprint(lpcmd, "lp -H -d%s", ARGF());
		break;
	case 'p':
		dopage = 1;
		break;
	case 'f':
		file = ARGF();
		break;
	} ARGEND

	if(argc > 0)
		arg = argv[0];
	slarg = strlen(arg);

	index = Bopen(file, OREAD);
	if(index == nil) {
		fprint(2, "cant open %s\n", file);
		exits(0);
	}

	Bseek(index, 8, 0);
	count = Bgetc(index);
	count |= Bgetc(index) << 8;
	count |= Bgetc(index) << 16;
	count |= Bgetc(index) << 24;

	offset = Bgetc(index);
	offset |= Bgetc(index) << 8;
	offset |= Bgetc(index) << 16;
	offset |= Bgetc(index) << 24;
//	print("offset = %.8lux; count = %d\n", offset, count);
	Bseek(index, offset, 0);

	in = Bopen(file, OREAD);
	if(in == nil) {
		fprint(2, "cant open %s\n", file);
		exits(0);
	}

	inflateinit();
	for(i=0; i<count; i++) {
		if(Bread(index, &dir, 0x28) != 0x28) {
			fprint(2, "cant read index %d %x\n", i, i);
			exits(0);
		}
		if(memcmp(dir.name, arg, slarg) != 0)
			continue;
		sprint(name, "/tmp/%s", dir.name);
		out = Bopen(name, OWRITE);
		if(out == nil)
			sysfatal("creating %s: %r", name);
		offset = dir.xxx[2] | (dir.xxx[3]<<8) |
			(dir.xxx[4]<<16) | (dir.xxx[5]<<24);

		Bseek(in, offset, 0);
		n = inflatezlib(out, (int (*)(void*, void*, int))Bwrite, in, (int (*)(void*))Bgetc);
		if(n < 0) {
			print("%s %lld\n", flateerr(n), Boffset(in));
			break;
		}
		Bterm(out);

		if(dopage)
			print("/bin/aviation/jep2ps %s;page /tmp/jepout.ps;rm /tmp/jepout.ps; rm %s\n",
				name, name);
		else
			print("/bin/aviation/jep2ps %s;%s /tmp/jepout.ps;rm /tmp/jepout.ps; rm %s\n",
				name, lpcmd, name);
	}
	exits(0);
}
