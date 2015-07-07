
#include <u.h>
#include <libc.h>

enum {
	Njmps = 10000
};

void foo(void);

int
main(void)
{
	int i, njmp;
	int fail = 0;
	jmp_buf label;

	njmp = 0;
	while((njmp = setjmp(label)) < Njmps)
		longjmp(label, njmp+1);

	for(i = 0; i < nelem(label); i++)
		fprint(2, "label[%d] = %p\n", i, label[i]);
	fprint(2, "main: %p foo: %p\n", main, foo);

	if(njmp != Njmps)
		fail++;
	if(label[JMPBUFPC] < (uintptr_t)main)
		fail++;
	if(label[JMPBUFPC] > (uintptr_t)foo)
		fail++;
	if(label[JMPBUFSP] > (uintptr_t)&label[nelem(label)])
		fail++;
	if(label[JMPBUFSP] < 0x7fffffd00000)
		fail++;

	if(fail == 0){
		print("PASS\n");
		exits("PASS");
	}
	print("FAIL\n");
	exits("FAIL");
	return 0;
}

void
foo(void)
{
}
