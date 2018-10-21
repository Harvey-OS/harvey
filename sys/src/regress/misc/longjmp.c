
#include <u.h>
#include <libc.h>

enum {
	Njmps = 10000
};

void foo(void);

void
main(void)
{
	int i, njmp;
	int fail = 0;
	jmp_buf label;

	while((njmp = setjmp(label)) < Njmps) {
		longjmp(label, njmp+1);
	}

	for(i = 0; i < nelem(label); i++)
		fprint(2, "label[%d] = %p\n", i, label[i]);
	fprint(2, "main: %p foo: %p\n", main, foo);

	if(njmp != Njmps) {
		print("njmp is %d, wanted %d\n", njmp, Njmps);
		fail++;
	}
	if(label[JMPBUFPC] < (uintptr_t)main) {
		print("label[%d] is %p, which is < main(%p)\n",
			JMPBUFPC, label[JMPBUFPC], main);
		fail++;
	}
	if(label[JMPBUFPC] > (uintptr_t)foo) {
		print("label[%d] is %p, which is > foo(%p)\n",
			JMPBUFPC, label[JMPBUFPC], foo);
		fail++;
	}
	if(label[JMPBUFSP] > (uintptr_t)&label[nelem(label)]) {
		print("label[%d] is %p, which is > (uintptr_t)&label[nelem(label)](%p)\n",
			JMPBUFSP, label[JMPBUFSP], foo);
		fail++;
	}

	if(fail == 0){
		print("PASS\n");
		exits(nil);
	}
	print("FAIL (%d errors)\n", fail);
	exits("FAIL");
}

void
foo(void)
{
}
