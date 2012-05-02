gorka writes:
---
I have userspace on the gumstix [xscale, not omap].  The problem that
got me in trouble was that in lexception.s (or l.s),

	MOVM.DB.W [R0-R14], (R13)

works differently for this architecture (and probably for others, as
it is unclear how it should behave by reading the arm specs).  This
happens only for kernel faults as the others (syscall, user faults)
use MOVM.DB.W.S which uses the banked user registers.

The problem is that in this arch the value of R13 saved is the value
after R13 itself has been modified, whereas in the others (bitsy,
pico...), it was the value before.  Adding 4*15 to the stack before
the RFE solves the problem.
---

In fact, the 2005 ARM arch. ref. man. (ARM DDI 0100I) says, under STM (1),
that if Rn appears in the set of registers (and isn't the first one)
and .W is specified, the stored value of Rn is unpredictable.
The arm v7-ar arch. ref. man. says such usage is obsolete.
