#include <u.h>
#include <libc.h>

typedef struct Endian Endian;
struct Endian {
	ulong val;
	char *name;
};

uchar data[4] = {0x11, 0x22, 0x33, 0x44};

Endian elist[] = {
	0x11223344, "big",
	0x44332211, "little"
};

void
main(void)
{
	char *end;
	ulong l;
	Endian *e;

	end = "unknown";
	l = *(ulong*)data;
	for(e=elist; e<elist+nelem(elist); e++)
		if(e->val == l)
			end = e->name;

	print("%s\n", end);
}
