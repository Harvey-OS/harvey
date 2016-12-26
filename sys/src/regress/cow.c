/* cow tests copy on write.
 *
 * It does so by doing a getpid(), then a fork.  Child and parent set
 * an auto (i.e. on-stack) stack value to getpid() again.  If COW
 * works, they will see the right value: pre and post-fork getpid
 * values will be the same.  If COW does not work, they will see
 * different values in pre- and post-fork pid values, because the page
 * won't be copied since COW did not happen and one of them will see
 * the other's stack.  Writing this test is *really* a bit tricky. You
 * have to write it with minimal branches because if COW is not
 * working, then the return from fork can go to the wrong place
 * because the parent and child might have the same stack. Which
 * means, sadly, that one can print PASS and one can print FAIL. I
 * don't know any good way to fix this. But this program has diagnosed
 * the problem with the riscv port failing to run pipering correctly.
 * So maybe all we can do is run it by hand.
 */
#include <u.h>
#include <libc.h>

void
main(int argc, char *argv[])
{
	int i;
	int pid, fpid, cpid;

	pid = getpid();

	if ((cpid = fork()) < 0)
		sysfatal("fork failed: %r");

	fpid = getpid();

	// This test is racy but it's hard to do without a race.
	if (cpid && pid != fpid) {
		print("FAIL: pre-fork pid is %d, post-fork pid is %d\n", pid, fpid);
		exits("FAIL");
	}

	print("PASS\n");
	exits("PASS");
}
