#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

int backtrace_list(uintptr_t pc, uintptr_t fp, uintptr_t *pcs, size_t nr_slots)
{
	size_t nr_pcs = 0;
	while (fp && nr_pcs < nr_slots) {
		/* could put some sanity checks in here... */
		pcs[nr_pcs++] = pc;
		//iprint("PC %p FP %p\n", pc, fp);
		if (fp < KTZERO)
			break;
		/* PC becomes the retaddr - 1.  the -1 is to put our PC back inside the
		 * function that called us.  this was necessary in case we called as the
		 * last instruction in a function (would have to never return).  not
		 * sure how necessary this still is. */
		pc = *(uintptr_t*)(fp + sizeof(uintptr_t)) - 1;
		fp = *(uintptr_t*)fp;
	}
	return nr_pcs;
}

#if 0
void backtrace_frame(uintptr_t eip, uintptr_t ebp)
{
	char *func_name;
	#define MAX_BT_DEPTH 20
	uintptr_t pcs[MAX_BT_DEPTH];
	size_t nr_pcs = backtrace_list(eip, ebp, pcs, MAX_BT_DEPTH);

	for (int i = 0; i < nr_pcs; i++) {
		func_name = get_fn_name(pcs[i]);
		print("#%02d [<%p>] in %s\n", i + 1,  pcs[i], func_name);
		kfree(func_name);
	}
}

void backtrace(void)
{
	uintptr_t ebp, eip;
	ebp = read_bp();
	/* retaddr is right above ebp on the stack.  we subtract an additional 1 to
	 * make sure the eip we get is actually in the function that called us.
	 * i had a couple cases early on where call was the last instruction in a
	 * function, and simply reading the retaddr would point into another
	 * function (the next one in the object) */
	eip = *(uintptr_t*)(ebp + sizeof(uintptr_t)) - 1;
	/* jump back a frame (out of backtrace) */
	ebp = *(uintptr_t*)ebp;
	printk("Stack Backtrace on Core %d:\n", core_id());
	backtrace_frame(eip, ebp);
}
#endif
