.FP palatino
.TM
.TL
Plan 9 on the Mikrotik RB450G Routerboard
.AU
Geoff Collyer
.AI
.MH
.NH 1
Motivation
.LP
I ported Plan 9 to the Routerboard mainly to verify
that Plan 9's MIPS-related code
(compiler, assembler, loader,
.CW libmach ,
etc.) was still in working order and would
work on newer machines than the 1993-era ones that we last owned
(MIPS Magnum, SGI Challenge, Carrera and the like).
The verdict is that,
with a few surprising exceptions, the code still works on newish machines
(the MIPS 24K CPU in the Routerboard dates to about 2003 originally;
this revision is from about 2005).
So we now have a
machine on which to test MIPS executables.
.LP
The other reason I did the port was
as an incremental step toward
running Plan 9 on a MIPS64 machine (e.g., the dual-core, dual-issue
Cavium CN5020 in the Ubiquiti Edgerouter Lite 3).
.NH 1
The new MIPS world
.LP
These newer MIPS systems are aimed at embedded applications, so they
typically lack FPUs and may also lack L2 caches or have small TLBs;
the MIPS 24K in the Atheros 7161 SoC lacks FPU and L2 cache, and has a
16-entry TLB.
It is a MIPS32R2 architecture system and lacks the 64-bit instructions
of the R4000.
These new MIPS systems are still big-endian,
so provide a useful test case to expose byte-ordering bugs.
.NH 1
Plan 9 changes and additions
.NH 2
CPU Bug Workarounds
.LP
The Linux MIPS people cite MIPS 24K erratum 48:
3 consecutive stores lose data.
MIPS only distribute their errata lists under NDA and to their
corporate partners, so we have only the Linux report to go on.
The fix requires
.I both
write-through data cache and
no more than two consecutive single-word stores in all executables.
I have made a crude optional change to
.I vl
to generate a NOP before every third consecutive store.
The fix could be better, in particular the technique for
keeping stores out of branch delay slots.
.NH 2
Driver for Undocumented Ethernet Controller
.LP
The FreeBSD Atheros
.I arge
driver
(in
.CW /usr/src/sys/mips/atheros )
provided inspiration for our Gigabit Ethernet driver, since the
hardware is otherwise largely undocumented.
I haven't got the second
Ethernet controller entirely working yet;
it's perhaps complicated by having a switch attached to it (the Atheros 8316).
At minimum, it probably needs MII or PHY initialisation.
.NH 2
Floating-point Emulation
.LP
Floating-point emulation works but is
.I very
slow:
.I astro
takes about 8 seconds.
I added an
.CW fpemudebug
command to
.CW /dev/archctl ;
it
takes a number as argument corresponding to the
.CW Dbg*
bits in
.CW fpimips.c ,
but requires the kernel to be compiled with
.CW FPEMUDEBUG
defined.
.NH 3
\&... in Locking Code
.LP
The big surprises included that
.CW /sys/src/libc/mips/lock.c
read
.CW FCR0
to
choose the locking style.
That's been broken out into
.CW c_fcr0.s
so that we can change it, but the kernel also emulates the
.CW MOVW
.CW FCR0,R1
(and via a fast code path), to keep alive the possibility of running
old binaries from the dump.
.NH 2
No 64-bit Instructions
.LP
The other big surprise was that
.CW /sys/src/libmp/mips/mpdigdiv.s
used 64-bit instructions (SLLV, SRLV, ADDVU, DIVVU).
For now I've resolved the problem by pushing it into a
subdirectory (\c
.CW r4k )
and editing the
.CW mkfile s
to use the
.CW port
version
(and similarly in APE).
.br
.ne 8
.NH 2
Page Size vs TLB Faults
.LP
I started out with a 4K page size and reduced the number of TLB
entries reserved for the kernel to 2, leaving 14 for user programs,
but
.CW /dev/sysstat
was reporting 6 times as many TLB faults as page
faults, and the number increased at a furious rate.
.LP
So I switched to
a 16K page size, adjusted
.CW vl
.CW -H2
accordingly and recompiled the
.CW /mips
world.
This reduced the TLB faults to just 10% more than the number of page faults.
(That number is now around 15% more, due to a better soft-TLB hash function
that makes the soft TLB more effective.)
16K pages also produce consecutive (even recursive) page faults
for the same address at the same PC
and the system runs at about 10% of its normal speed,
so 4K pages are currently the only sensible choice;
we'll just live with the absurdly-high number of TLB faults
(around 20k–30k per second).
It probably doesn't help that one 16K page is half of the L1 data cache
and one quarter of the L1 instruction cache.
.LP
Page size is controlled by
.CW BIGPAGES
in
.CW mem.h .
.NH 3
Combined TLB Pool
.LP
I also changed
.CW mmu.c
to collapse the separate kernel and user TLB pools into one,
once user processes start running,
but that only helps to reduce TLB faults a little.
.
.br
.ne 8
.
.NH 1
Remaining Problems
.LP
Interrupt-driven UART output isn't quite right.
It can get stuck and then input makes it resume.
The UART is apparently connected via the APB and requires
interrupt unmasking in the APB (which we now do).
There's some kludgey stuff in
.CW uarti8250.c
that makes output work most of the time
(characters do sometimes get dropped).
.LP
The Ethernet driver currently does not
dig out the MAC addresses from the hardware,
so you'll need to edit the
.CW rb
configuration file for each Routerboard; the format should be obvious.
I don't have the stomach to dig the MAC address out of the hardware
via SPI or whatever vile interface it requires.
