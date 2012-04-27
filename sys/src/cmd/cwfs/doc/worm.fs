# fs: new main file server on 633MHz machine with 4 IDE disks & 4K blocks
# was called rosewood at first
config h0
service fs
ip0     66.120.90.177
ipgw0   66.120.90.186
ipmask0 255.255.255.0
ipsntp  66.120.90.185
filsys main c{p(h0)0.25p(h2)0.25}f{p(h0)25.75p(h2)25.75}
filsys dump o
filsys other h3
# later added: filsys spare h1
# ream other
# ream main
# recover main
allow
end

h0 20GB "main":  25% cache and 75% fake-worm;
h1 40GB "spare": holds test venti used for backup
h2 20GB "main":  (h1.0.0) a mirror of h0.
h3 40GB "other": (h1.1.0) ``scratch'' space

---
halted. press a key to reboot.

ether#0: i82557: port 0x9400 irq 10: 00A0C9E02756
dev A0 port 1F0 config 427A capabilities 2F00 mwdma 0007 udma 103F
dev A0 port 170 config 427A capabilities 2F00 mwdma 0007 udma 043F
dev B0 port 170 config 045A capabilities 0F00 mwdma 0007 udma 043F
found 9PCFSIMR.    attr 0x0 start 0x423 len 391193
.239705.............................+41712.....+181524=462941
entry: 0x80100020
cpu0: 635MHz GenuineIntel PentiumIII/Xeon (cpuid: AX 0x0686 DX 0x383f9ff)
ether0: i82557: 100Mbps port 0x9400 irq 10: 00a0c9e02756

iobufinit
        114253 buffers; 14281 hashes
        mem left = 44040191
                out of = 528482304
nvr read
found plan9.nvr attr 0x0 start 0x18c len 677
for config mode hit a key within 5 seconds

config: filsys main c{p(h0)0.25p(h2)0.25}f{p(h0)25.75p(h2)25.75}
config: filsys dump o
config: filsys other h3
config: recover main 
config: ream other
config: allow
config: end
ysinits
config h0
        devinit h0
i hd0: DW CDW02E0-B00HB0F    lba/atapi/drqintr: 1/0/0  C/H/S: 0/0/0  CAP: 39102336
hd0: LBA 39102336 sectors
ideinit(device 9ce00948 ctrl 0 targ 0) driveno 0 dp 802eff24
config block written
config h0
        devinit h0
ideinit(device 9ce00988 ctrl 0 targ 0) driveno 0 dp 802eff24
service    rosewood
ipauth  0.0.0.0
ipsntp  66.120.90.185
ip0     66.120.90.194
ipgw0   66.120.90.186
ipmask0 255.255.255.0
filsys main c{p(h0)0.25p(h2)0.25}f{p(h0)25.75p(h2)25.75}
filsys dump o
filsys other h3
sysinit: main
recover: c{p(h0)0.25p(h2)0.25}f{p(h0)25.75p(h2)25.75}
        devinit {p(h0)0.25p(h2)0.25}
        devinit p(h0)0.25
        devinit h0
ideinit(device 9ce00a68 ctrl 0 targ 0) driveno 0 dp 802eff24
atasize(9ce00a68):  39102336 -> 4887552
        devinit p(h2)0.25
        devinit h2
i hd2: DW CDW02E0-B00HB0F    lba/atapi/drqintr: 1/0/0  C/H/S: 0/0/0  CAP: 39102336
hd2: LBA 39102336 sectors
ideinit(device 9ce00ae8 ctrl 0 targ 2) driveno 2 dp 802f4324
atasize(9ce00ae8):  39102336 -> 4887552
        devinit f{p(h0)25.75p(h2)25.75}
fworm init
        devinit {p(h0)25.75p(h2)25.75}
        devinit p(h0)25.75
        devinit h0
ideinit(device 9ce00bc8 ctrl 0 targ 0) driveno 0 dp 802eff24
atasize(9ce00bc8):  39102336 -> 4887552
        devinit p(h2)25.75
        devinit h2
ideinit(device 9ce00c48 ctrl 0 targ 2) driveno 2 dp 802f4324
atasize(9ce00c48):  39102336 -> 4887552
dump 2 is good; 5 next
dump 5 is good; 494408 next
dump 494408 is good; 495193 next
dump 495193 is good; 495224 next
dump 495224 is good; 496007 next
dump 496007 is good; 496062 next
dump 496062 is good; 496089 next
dump 496089 is good; 496096 next
dump 496096 is good; 496118 next
dump 496118 is good; 496882 next
fworm: read 496882
cache init c{p(h0)0.25p(h2)0.25}f{p(h0)25.75p(h2)25.75}
done cacheinit
done recover
        devinit c{p(h0)0.25p(h2)0.25}f{p(h0)25.75p(h2)25.75}
sysinit: dump
        devinit o{p(h0)0.25p(h2)0.25}f{p(h0)25.75p(h2)25.75}
sysinit: other
        devream: h3 1
        devinit h3
i hd3: AMTXRO4 0K042H        lba/atapi/drqintr: 1/0/0  C/H/S: 0/0/0  CAP: 78198750
hd3: LBA 78198750 sectors
ideinit(device 9ce00ca8 ctrl 0 targ 3) driveno 3 dp 802f44f0
atasize(9ce00ca8):  78198750 -> 9774592
ether0o: 00a0c9e02756 66.120.90.194
ether0i: 00a0c9e02756 66.120.90.194
next dump at Mon Sep 10 05:00:00 2001
current fs is "main"
il: allocating il!66.120.90.189!23230
41 uids read, 21 groups used
rosewood as of Sun Sep  9 16:27:27 2001
        last boot Mon Sep 10 00:56:10 2001
touch superblock 496118
rosewood: 
