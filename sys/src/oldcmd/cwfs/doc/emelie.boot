Mon Feb 12 13:06:31: ether#0: i82557: port 0xEC00 irq 10: 00A0C90AC853
Mon Feb 12 13:06:31: Boot devices: fd0 ether0
Mon Feb 12 13:06:33: boot from: ether0
Mon Feb 12 13:06:33: lookout (135.104.9.7!67): /386/9pcfs
Mon Feb 12 13:06:33: 299219+119508+190052=608779
Mon Feb 12 13:06:33: entry: 0x80100020
Mon Feb 12 13:06:35: cpu0: 267MHz GenuineIntel PentiumII (cpuid: AX 0x0634 DX 0x80f9ff)
Mon Feb 12 13:06:36: ether0: i82557: 100Mbps port 0xec00 irq 10: 00a0c90ac853
Mon Feb 12 13:06:37: scsi#0: buslogic: port 0x334 irq 11
Mon Feb 12 13:06:37: scsi#1: buslogic: port 0x330 irq 15
Mon Feb 12 13:06:39: 
Mon Feb 12 13:06:39: iobufinit
Mon Feb 12 13:06:39: bank[0]: base 0x0009fc00, limit 0x0009fc00
Mon Feb 12 13:06:39: bank[1]: base 0x01298130, limit 0x10000000
Mon Feb 12 13:06:40: 	13779 buffers; 1723 hashes
Mon Feb 12 13:06:41: 	mem left = 22380543
Mon Feb 12 13:06:41: 		out of = 268435456
Mon Feb 12 13:06:41: nvr read
Mon Feb 12 13:06:42: found plan9.nvr attr 0x0 start 0x25a len 64064
Mon Feb 12 13:06:42: for config mode hit a key within 5 seconds
Mon Feb 12 13:06:48: 	no config
Mon Feb 12 13:06:48: sysinit
Mon Feb 12 13:06:48: config w1.0.0
Mon Feb 12 13:06:48: 	devinit w1.0.0
Mon Feb 12 13:06:48: scsi#1.0: SEAGATE ST39173WC       4218LM387268
Mon Feb 12 13:06:48: 	drive w1.0.0:
Mon Feb 12 13:06:48: 		17783239 blocks at 512 bytes each
Mon Feb 12 13:06:48: 		555726 logical blocks at 16384 bytes each
Mon Feb 12 13:06:48: 		32 multiplier
Mon Feb 12 13:06:48: service    emelie
Mon Feb 12 13:06:48: ipauth  135.104.9.7
Mon Feb 12 13:06:48: ipsntp  135.104.9.52
Mon Feb 12 13:06:48: ip0     135.104.9.42
Mon Feb 12 13:06:48: ipgw0   135.104.9.1
Mon Feb 12 13:06:48: ipmask0 255.255.255.0
Mon Feb 12 13:06:48: filsys main c[w1.<0-5>.0]j(w6w5w4w3w2)(l<0-236>l<238-474>)
Mon Feb 12 13:06:48: filsys dump o
Mon Feb 12 13:06:48: filsys other [w1.<9-14>.0]
Mon Feb 12 13:06:48: filsys temp [w1.8.0]
Mon Feb 12 13:06:51: sysinit: main
Mon Feb 12 13:06:51: 	devinit c[w1.0.0-w1.5.0]j(w6-w2)(l0-l474)
Mon Feb 12 13:06:51: 	devinit [w1.0.0-w1.5.0]
Mon Feb 12 13:06:51: 	devinit w1.0.0
Mon Feb 12 13:06:51: 	drive w1.0.0:
Mon Feb 12 13:06:51: 		17783239 blocks at 512 bytes each
Mon Feb 12 13:06:51: 		555726 logical blocks at 16384 bytes each
Mon Feb 12 13:06:51: 		32 multiplier
Mon Feb 12 13:06:51: 	devinit w1.1.0
Mon Feb 12 13:06:51: scsi#1.1: SEAGATE ST39173WC       6244LM399207
Mon Feb 12 13:06:51: 	drive w1.1.0:
Mon Feb 12 13:06:51: 		17783239 blocks at 512 bytes each
Mon Feb 12 13:06:51: 		555726 logical blocks at 16384 bytes each
Mon Feb 12 13:06:51: 		32 multiplier
Mon Feb 12 13:06:51: 	devinit w1.2.0
Mon Feb 12 13:06:51: scsi#1.2: SEAGATE ST39173WC       4218LMJ04565
Mon Feb 12 13:06:51: 	drive w1.2.0:
Mon Feb 12 13:06:51: 		17783239 blocks at 512 bytes each
Mon Feb 12 13:06:51: 		555726 logical blocks at 16384 bytes each
Mon Feb 12 13:06:51: 		32 multiplier
Mon Feb 12 13:06:51: 	devinit w1.3.0
Mon Feb 12 13:06:51: scsi#1.3: SEAGATE ST39173WC       4218LM412029
Mon Feb 12 13:06:51: 	drive w1.3.0:
Mon Feb 12 13:06:51: 		17783239 blocks at 512 bytes each
Mon Feb 12 13:06:51: 		555726 logical blocks at 16384 bytes each
Mon Feb 12 13:06:52: 		32 multiplier
Mon Feb 12 13:06:52: 	devinit w1.4.0
Mon Feb 12 13:06:52: scsi#1.4: SEAGATE ST39173WC       4218LM412834
Mon Feb 12 13:06:52: 	drive w1.4.0:
Mon Feb 12 13:06:52: 		17783239 blocks at 512 bytes each
Mon Feb 12 13:06:52: 		555726 logical blocks at 16384 bytes each
Mon Feb 12 13:06:52: 		32 multiplier
Mon Feb 12 13:06:52: 	devinit w1.5.0
Mon Feb 12 13:06:52: scsi#1.5: SEAGATE ST39173WC       4218LM364585
Mon Feb 12 13:06:52: 	drive w1.5.0:
Mon Feb 12 13:06:52: 		17783239 blocks at 512 bytes each
Mon Feb 12 13:06:52: 		555726 logical blocks at 16384 bytes each
Mon Feb 12 13:06:52: 		32 multiplier
Mon Feb 12 13:06:52: 	devinit j(w6-w2)(l0-l474)
Mon Feb 12 13:06:52: alloc juke w6
Mon Feb 12 13:06:52: scsi#0.6: HP      C1107J          1.50
Mon Feb 12 13:06:52: 	mt 16 2
Mon Feb 12 13:06:52: 	se 31 238
Mon Feb 12 13:06:52: 	ie 20 1
Mon Feb 12 13:06:52: 	dt 1 4
Mon Feb 12 13:06:52: 	rot 1
Mon Feb 12 13:07:02: import/export 0 #38 0.0
Mon Feb 12 13:07:02: data transfer 0 #08 0.0
Mon Feb 12 13:07:02: data transfer 1 #08 0.0
Mon Feb 12 13:07:02: data transfer 2 #08 0.0
Mon Feb 12 13:07:02: data transfer 3 #08 0.0
Mon Feb 12 13:07:02: 	shelves r0-r237
Mon Feb 12 13:07:02: 	load   r0 drive w2
Mon Feb 12 13:07:10: scsi#0.2: HP      C1113F          1.129807307965    8XMO  
Mon Feb 12 13:07:10: 	worm l0: drive w2
Mon Feb 12 13:07:10: 		1263471 blocks at 2048 bytes each
Mon Feb 12 13:07:10: 		157934 logical blocks at 16384 bytes each
Mon Feb 12 13:07:10: 		8 multiplier
Mon Feb 12 13:07:10: label l0 ordinal 0
Mon Feb 12 13:07:18: sysinit: dump
Mon Feb 12 13:07:18: 	devinit o[w1.0.0-w1.5.0]j(w6-w2)(l0-l474)
Mon Feb 12 13:07:21: sysinit: other
Mon Feb 12 13:07:21: 	devinit [w1.9.0-w1.14.0]
Mon Feb 12 13:07:21: 	devinit w1.9.0
Mon Feb 12 13:07:21: scsi#1.9: SEAGATE ST39173WC       4218LM367449
Mon Feb 12 13:07:21: 	drive w1.9.0:
Mon Feb 12 13:07:21: 		17783239 blocks at 512 bytes each
Mon Feb 12 13:07:21: 		555726 logical blocks at 16384 bytes each
Mon Feb 12 13:07:21: 		32 multiplier
Mon Feb 12 13:07:21: 	devinit w1.10.0
Mon Feb 12 13:07:21: scsi#1.10: SEAGATE ST39173WC       4218LM392062
Mon Feb 12 13:07:21: 	drive w1.10.0:
Mon Feb 12 13:07:21: 		17783239 blocks at 512 bytes each
Mon Feb 12 13:07:21: 		555726 logical blocks at 16384 bytes each
Mon Feb 12 13:07:21: 		32 multiplier
Mon Feb 12 13:07:21: 	devinit w1.11.0
Mon Feb 12 13:07:21: scsi#1.11: SEAGATE ST39173WC       4218LMB43130
Mon Feb 12 13:07:21: 	drive w1.11.0:
Mon Feb 12 13:07:22: 		17783239 blocks at 512 bytes each
Mon Feb 12 13:07:22: 		555726 logical blocks at 16384 bytes each
Mon Feb 12 13:07:22: 		32 multiplier
Mon Feb 12 13:07:22: 	devinit w1.12.0
Mon Feb 12 13:07:22: scsi#1.12: SEAGATE ST39173WC       4218LMB38636
Mon Feb 12 13:07:22: 	drive w1.12.0:
Mon Feb 12 13:07:22: 		17783239 blocks at 512 bytes each
Mon Feb 12 13:07:22: 		555726 logical blocks at 16384 bytes each
Mon Feb 12 13:07:22: 		32 multiplier
Mon Feb 12 13:07:22: 	devinit w1.13.0
Mon Feb 12 13:07:22: scsi#1.13: SEAGATE ST39173WC       4218LM391739
Mon Feb 12 13:07:22: 	drive w1.13.0:
Mon Feb 12 13:07:22: 		17783239 blocks at 512 bytes each
Mon Feb 12 13:07:22: 		555726 logical blocks at 16384 bytes each
Mon Feb 12 13:07:22: 		32 multiplier
Mon Feb 12 13:07:22: 	devinit w1.14.0
Mon Feb 12 13:07:22: scsi#1.14: SEAGATE ST39173WC       4218LM408443
Mon Feb 12 13:07:22: 	drive w1.14.0:
Mon Feb 12 13:07:22: 		17783239 blocks at 512 bytes each
Mon Feb 12 13:07:22: 		555726 logical blocks at 16384 bytes each
Mon Feb 12 13:07:22: 		32 multiplier
Mon Feb 12 13:07:24: sysinit: temp
Mon Feb 12 13:07:24: 	devinit w1.8.0
Mon Feb 12 13:07:24: scsi#1.8: SEAGATE ST39173WC       4218LMC15632
Mon Feb 12 13:07:24: 	drive w1.8.0:
Mon Feb 12 13:07:24: 		17783239 blocks at 512 bytes each
Mon Feb 12 13:07:24: 		555726 logical blocks at 16384 bytes each
Mon Feb 12 13:07:24: 		32 multiplier
Mon Feb 12 13:07:24: ether0o: 00a0c90ac853 135.104.9.42
Mon Feb 12 13:07:24: ether0i: 00a0c90ac853 135.104.9.42
Mon Feb 12 13:07:25: next dump at Tue Feb 13 05:00:00 2007
Mon Feb 12 13:07:25: current fs is "main"
Mon Feb 12 13:07:25: 582 uids read, 189 groups used
Mon Feb 12 13:07:25: emelie as of Wed May  3 12:58:03 2006
Mon Feb 12 13:07:25: 	last boot Mon Feb 12 13:06:03 2007
Mon Feb 12 13:07:25: 	load   r226 drive w3
Mon Feb 12 13:07:33: scsi#0.3: HP      C1113J          1.129809312612    8XMO  
Mon Feb 12 13:07:33: 	worm l226: drive w3
Mon Feb 12 13:07:33: 		1263471 blocks at 2048 bytes each
Mon Feb 12 13:07:33: 		157934 logical blocks at 16384 bytes each
Mon Feb 12 13:07:33: 		8 multiplier
Mon Feb 12 13:07:33: label l226 ordinal 226
Mon Feb 12 13:07:33: j(w6-w2)(l0-l474) touch superblock 35709856
Mon Feb 12 13:07:35: emelie: version
Mon Feb 12 13:07:36: emelie as of Wed May  3 12:58:03 2006
Mon Feb 12 13:07:36: 	last boot Mon Feb 12 13:06:03 2007
Mon Feb 12 13:07:44: emelie: printconf
Mon Feb 12 13:07:44: config w1.0.0
Mon Feb 12 13:07:44: service emelie
Mon Feb 12 13:07:44: filsys main c[w1.<0-5>.0]j(w6w5w4w3w2)(l<0-236>l<238-474>)
Mon Feb 12 13:07:44: filsys dump o
Mon Feb 12 13:07:44: filsys other [w1.<9-14>.0]
Mon Feb 12 13:07:44: filsys temp [w1.8.0]
Mon Feb 12 13:07:44: ipauth 135.104.9.7
Mon Feb 12 13:07:44: ipsntp 135.104.9.52
Mon Feb 12 13:07:44: ip0 135.104.9.42
Mon Feb 12 13:07:44: ipgw0 135.104.9.1
Mon Feb 12 13:07:44: ipmask0 255.255.255.0
Mon Feb 12 13:07:44: end
Mon Feb 12 13:07:48: emelie: date
Mon Feb 12 13:07:48: Mon Feb 12 13:07:03 2007 + 0
Mon Feb 12 13:08:48: emelie: sntp: 1171303728
Mon Feb 12 13:17:14: 	time   r0 drive w2
Mon Feb 12 13:17:39: 	time   r226 drive w3
