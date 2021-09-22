
Plan 9 from Bell Labs (mp arm)

l1: int split i&d, 4 ways 256 sets 32 bytes/line; can WB; can write-allocate; l1 I policy VIPT
l2: ext unified,   8 ways 512 sets 32 bytes/line; can WT; can WB; can write-allocate
fp: arm arch VFPv3+ with null subarch
fastclock: 125047/501µs = 250 fastticks/µs (MHz)
1000 mips (single-issue), 1980 mips (dual-issue)
cpu0: 1000MHz ARM Cortex-A9 r1p0
rtl8169: 64-bit register accesses
rtl8169: oui 0x732 phyno 1, macv = 0x28000000 phyv = 0x0002
#l0: rtl8169: 1000Mbps port 0xa0000000 irq 130: 0001c00bfb98
1019M memory: 205M kernel data, 813M user, 3866M swap
old evp 0xe089dc
cpu1 on.
cpu0: scheduling
cpu1 starting
cpu1: 1000MHz ARM Cortex-A9 r1p0
cpu1: scheduling
r1link up...version...time...
secstore: null password, skipping secstore login

init: starting /bin/rc
ts# date
date
Fri Feb 17 15:57:30 EST 2012
ts# cat /dev/sysstat
cat /dev/sysstat
          0       10108      546486       10873        7123           0           0         171          87           0 
          1           0           1           0           0           0           0           0           0           0 
ts# uptime
uptime
ts up 0 days, 00:01:46
ts# cat /dev/sysstat
cat /dev/sysstat
          0       10980      656827       11659        8067           0           0         213          87           0 
          1           0           1           0           0           0           0           0           0           0 
ts# cat /dev/sysstat
cat /dev/sysstat
          0       11062      684752       11685        8135           0           0         222          87           0 
          1           0           1           0           0           0           0           0           0           0 
ts# 
