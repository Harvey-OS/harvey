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
	char p[16];

	if(((usize)&p & 15) != 0){
		print("%p not 16-byte aligned\n", &p);
		exits("not 16-byte aligned");
	}

	print("PASS\n");
	exits(nil);
}
