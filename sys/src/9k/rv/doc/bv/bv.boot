Starting kernel ...

asmalloc: 0x0000000000124000 @ 0x0000000080000000, type 0
asmfree:  0x0000000080000000 @ 0x0000000000000000, type 0
asmfree: insert 2 0x0000000080000000 @ 0x0000000000000000, type 0
asmmapinit 0x0000000080000000 0x000000003ff00000 mem
asmalloc: 0x000000003fddc000 @ 0x0000000080124000, type 0
asmfree:  0x000000003fddc000 @ 0x0000000080124000, type 1
asmfree: insert 2 0x000000003fddc000 @ 0x0000000080124000, type 1
asmmapinit 0x0000000100000000 0x0000000180000000 mem
asmalloc: 0x0000000180000000 @ 0x0000000100000000, type 0
asmfree:  0x0000000040100000 @ 0x00000000bff00000, type 0
asmfree: insert 2 0x0000000040100000 @ 0x00000000bff00000, type 0
asmfree:  0x0000000180000000 @ 0x0000000100000000, type 1
asmfree: insert 2 0x0000000180000000 @ 0x0000000100000000, type 1

Plan 9 (risc-v RV64G)
39-bit virtual addresses in use
cpu0: risc-v RV64G at 1000 MHz
1 of 16 l2 cache ways enabled at entry; setting to 15
31,251 clint ticks is 5,000,070 rdtsc cycs, or 159 cycs/clint
timebase given as 6,250,000
asmalloc: 0x00000000000dc000 @ 0x0000000000000000, type 1
asmalloc: 0x000000000fdff000 @ 0x0000000080200000, type 1
meminit: mem 0x0000000080200000
map kzero...map kseg2 to extra ram
kseg2 pa 0x000000008ffff000 end 0x00000000bff00000 mem (804261888) start at va 0x8ffff000
kseg2 pa 0x0000000100000000 end 0x0000000280000000 mem (6442450944) start at va 0x100000000
populate palloc.mem
asm base 0x8ffff000 lim 0xbff00000 for palloc.mem[0]
asm base 0x100000000 lim 0x280000000 for palloc.mem[1]
allocate Page array
kernel stops at 0x00000000bff00000 3,220,176,896
alloc(226,463,744) for 1,769,217 Pages
allocpages
Pages allocated at 0xffffffc08ffff000 (226,463,744 bytes)
asm pm0: base 0x000000009d7f8000 limit 0x00000000bff00000 npage 141064
asm pm1: base 0x0000000100000000 limit 0x0000000280000000 npage 1572864
#l0: syngmac: 1Gbps regs 0x10020000 irq 6: 2cf7f11be3cb
sbi starting hart 0
1 2 cpus running
palloc[0] col 0 0x000000009d7f8000 0x00000000bff00000
palloc[1] col 0 0x0000000100000000 0x0000000280000000
7165M memory: 1040K+464M kernel, 6695M user, 5M unused
ether interrupt when disabled
boot...dhcp...factotum...can't open #S/sdM0/nvram: '/env/nvroff' file does not exist
authid: 
