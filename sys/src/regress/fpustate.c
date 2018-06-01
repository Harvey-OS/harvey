#include <u.h>
#include <libc.h>

// Test that the FPU state is correctly saved and available in the /proc/<pid>/fpregs

void
pass(void) {
	print("PASS\n");
	exits("PASS");
}

void
fail(const char *msg) {
	print("FAIL - %s\n", msg);
	exits("FAIL");
}

void
main(void)
{
	int childpid = fork();
	if (childpid > 0) {
		// Sleep a little to ensure the child is in the infinite loop,
		// then read the fpregs and kill the child

		const char *path = smprint("/proc/%d/ctl", childpid);
		int ctlfd = open(path, OWRITE);
		if (ctlfd < 0) {
			fprint(ctlfd, "kill");
			fail("Can't open ctl");
		}

		sleep(1000);

		path = smprint("/proc/%d/fpregs", childpid);
		int fpfd = open(path, OREAD);
		if (fpfd < 0) {
			fprint(ctlfd, "kill");
			fail("Can't open fpregs");
		}

		// Read fpregs - it would be better to read individual registers
		// from the file system - update this when it happens
		const int buflen = 512;
		char fpregbuf[buflen];
		if (pread(fpfd, fpregbuf, buflen, 0) < buflen) {
			fprint(ctlfd, "kill");
			fail("Can't read fpregs");
		}

		// xmm0 starts at 160, each xmm reg at 16 byte increments
		int xmm0 = *(int*)(fpregbuf + 160);
		int xmm1 = *(int*)(fpregbuf + 176);
		int xmm2 = *(int*)(fpregbuf + 192);
		int xmm3 = *(int*)(fpregbuf + 208);
		if (xmm0 != 1 && xmm1 != 2 && xmm2 != 3 && xmm3 != 4) {
			fprint(ctlfd, "kill");
			fail("unexpected values in child's fpreg buffer");
		}

		fprint(ctlfd, "kill");
		pass();

	} else if (childpid == 0) {
		// In child

		// Set some xmm variables then run a tight loop waiting for
		// the parent to read the fpregs and kill the child
		__asm__ __volatile__(
			"movq $1, %%rax;"
			"movq %%rax, %%xmm0;"
			"movq $2, %%rax;"
			"movq %%rax, %%xmm1;"
			"movq $3, %%rax;"
			"movq %%rax, %%xmm2;"
			"movq $4, %%rax;"
			"movq %%rax, %%xmm3;"
			"loop:"
			"jmp loop;"
			: : : "rax", "xmm0", "xmm1", "xmm2", "xmm3");

		fail("Somehow exited the asm loop");

	} else {
		// Error
		fail("fork failed");
	}
}
