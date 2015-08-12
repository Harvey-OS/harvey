#include <u.h>
#include <libc.h>

/*
 *	The whole regression test I am after here is to call regress/args with
 *	arguments of various lengths, in order to trigger different stack alignments
 *	due to varying amounts of stuff in args.
 *
 *	It turned out that gcc compiles fprintf into something that uses
 *	fpu instructions which require the stack to be 16-aligned, so in
 *	fact the fprint for sum here would suicide the process if the stack
 *	it got happened to be not 16-aligned.
 *
 */

void
main(int argc, char *argv[])
{
	char *p;
	int i;
	double sum;

	if(((uintptr_t)&p & 15) != 0) {
		fprint(2, "%p not 16-aligned\n", &p);
		print("FAIL\n");
		exits("FAIL");
	}

	sum = 0.0;
	for(i = 0; i < argc; i++) {
		p = argv[i];
		sum += strtod(p, nil);
	}
	fprint(2, "&sum %p\n", &sum);
	fprint(2, "sum %f\n", sum);
	print("PASS\n");
	exits("PASS");
}
