Starting kernel ...

cpu0: main called from ffffffc080000638, hart count 4


Plan 9 (risc-v RV64G)
39-bit virtual addresses in use
cpu0: risc-v RV64G at 600 MHz
8 of 16 l2 cache ways enabled at entry; setting to 15
10,000 clint ticks is 6,000,096 rdtsc cycs, or 600 cycs/clint
mach0: ffffffc0bff87000 npgsz 3
allocinit: pmstart 8010a000 pmbase 80000000
mmuinit: kseg0: vmstart ffffffc080000000 vmunused ffffffc08010a000
	vmunmapped ffffffc080200000 vmend ffffffc090000000
mach cpu0: m ffffffc0bff87000; pt root va ffffffc0bff06000 -> pa bff06000
KZERO lvl 2 -> l 2 ffffffc0bff06810 200001ef
KZERO lvl 1 -> l 2 ffffffc0bff06810 200001ef
KZERO lvl 0 -> l 2 ffffffc0bff06810 200001ef
vmap(2000000, 65536)
vmap(2000000, 65536) => va ffffffffc0000000
vmap(2010000, 4096)
vmap(2010000, 4096) => va ffffffffc0010000
vmap(70000000, 67108864)
vmap(70000000, 67108864) => va ffffffffc0200000
vmap(3004000, 32768)
vmap(3004000, 32768) => va ffffffffc0011000
vmap(c000000, 67108864)
vmap(c000000, 67108864) => va ffffffffc4200000
vmap(20100000, 4096)
vmap(20100000, 4096) => va ffffffffc0019000
sbi extensions present: 0x0 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x8 0x10 0x735049 0x52464e43
trapinit...printinit...pcireset...timersinit...clocksanity...links...devtabreset...vmap(20112000, 8192)
vmap(20112000, 8192) => va ffffffffc001a000
vmap(20110000, 8192)
vmap(20110000, 8192) => va ffffffffc001c000
#l0: gem: 1Gbps regs 0x20112000 irq 70: 56341200fc00
#l1: gem: 1Gbps regs 0x20110000 irq 64: 56341200fc01
61 on w mach ffffffc08218c000...cpu1: main called from ffffffc080000638, hart count 4
1 Q1 2 on w mach ffffffc082215000...mmuinitap: on...cpu2: main called from ffffffc080000638, hart count 4
2 Q2 3 on w mach ffffffc08229e000...mach cpu1: m ffffffc08218c000; pt root va ffffffc08210b000 -> pa 8210b000
3 mmuinitap: on...4 cpus running
cpu3: main called from ffffffc080000638, hart count 4
mach cpu2: m ffffffc082215000; pt root va ffffffc082194000 -> pa 82194000
Q3 pageinit...zero user...zero user...
mmuinitap: on...
Hello Squidboy 2
mach cpu3: m ffffffc08229e000; pt root va ffffffc08221d000 -> pa 8221d000
mach 2 waiting for epoch
zero user...mach 2 is go ffffffc082215000 ffffffc082194000
Hello Squidboy 1
mach2: waiting for online flag

W2 Hello Squidboy 3
C2 mach 3 waiting for epoch
mach 1 waiting for epoch
mach 3 is go ffffffc08229e000 ffffffc08221d000
mach2: online color 0
mach3: waiting for online flag
mach 1 is go ffffffc08218c000 ffffffc08210b000
W3 mach1: waiting for online flag
C3 W1 mach3: online color 0
C1 mach1: online color 0
userinit...schedinit 0...cpu1: schedinit...cpu2: schedinit...cpu3: schedinit...
