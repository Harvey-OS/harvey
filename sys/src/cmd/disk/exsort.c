#include	<u.h>
#include	<libc.h>

int	ulcmp(void*, void*);
void	swapem(ulong*, long);

enum
{
	Wormsize	= 157933,
};
int	wflag;

void
main(int argc, char *argv[])
{
	long i, l, x, lobits, hibits, tot;
	int f, j;
	char *file;
	ulong *b, a, lo, hi;

	ARGBEGIN {
	default:
		print("usage: disk/exsort [-w] [file]\n");
		exits("usage");
	case 'w':
		wflag++;
		break;
	} ARGEND;

	file = "/adm/cache";
	if(argc > 0)
		file = argv[0];

	if(wflag)
		f = open(file, ORDWR);
	else
		f = open(file, OREAD);
	if(f < 0) {
		print("cant open %s: %r\n", file);
		exits("open");
	}
	l = seek(f, 0, 2) / sizeof(long);

	b = malloc(l*sizeof(long));
	if(b == 0) {
		print("cant malloc %s: %r\n", file);
		exits("malloc");
	}
	seek(f, 0, 0);
	if(read(f, b, l*sizeof(long)) != l*sizeof(long)) {
		print("short read %s: %r\n", file);
		exits("read");
	}

	lobits = 0;
	hibits = 0;
	for(i=0; i<l; i++) {
		a = b[i];
		if(a & (1L<<7))
			lobits++;
		if(a & (1L<<31))
			hibits++;
	}

	print("lobits = %6ld\n", lobits);
	print("hibits = %6ld\n", hibits);

	if(hibits > lobits) {
		print("swapping\n");
		swapem(b, l);
	}

	qsort(b, l, sizeof(ulong), ulcmp);

	tot = 0;
	for(j=0; j<100; j++) {
		lo = j*Wormsize;
		hi = lo + Wormsize;

		x = 0;
		for(i=0; i<l; i++) {
			a = b[i];
			if(a >= lo && a < hi)
				x++;
		}
		if(x) {
			print("disk %2d %6ld blocks\n", j, x);
			tot += x;
		}
	}
	print("total   %6ld blocks\n", tot);


	if(wflag) {
		if(hibits > lobits)
			swapem(b, l);
		seek(f, 0, 0);
		if(write(f, b, l*sizeof(long)) != l*sizeof(long)) {
			print("short write %s\n", file);
			exits("write");
		}
	}

	exits(0);
}

int
ulcmp(void *va, void *vb)
{
	ulong *a, *b;

	a = va;
	b = vb;

	if(*a > *b)
		return 1;
	if(*a < *b)
		return -1;
	return 0;
}

void
swapem(ulong *b, long l)
{
	long i;
	ulong x, a;

	for(i=0; i<l; i++, b++) {
		a = *b;
		x = (((a>>0) & 0xff) << 24) |
			(((a>>8) & 0xff) << 16) |
			(((a>>16) & 0xff) << 8) |
			(((a>>24) & 0xff) << 0);
		*b = x;
	}
}
