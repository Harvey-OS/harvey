.HTML "Setting Up Plan 9"
.TL
Setting Up Plan 9
.AU
Geoff Collyer
.AI
.MH
.AB
How to set up a clone of the MH Plan 9 system
from bare hardware and a stack of Blu-Ray backup discs.
.AE
.
.SH
Overview
.LP
This document assumes that you have essentially the same hardware
as we do in MH, plus a set of Blu-Ray discs containing our backups of our
.I venti
arenas.
.LP
The overall plan is to:
upgrade the firmware of each Intel SR1520ML system;
get your Coraid RAID array configured, partitioned and running;
get your
.I venti
store configured, loaded and running;
get your fossils configured and running;
and finally
set up your KVM and serial servers and power bars.
.
.SH
Firmware
.LP
To get the second Ethernet interface working well,
you'll need to apply this firmware update to each Intel SR1520ML system.
.LP
Insert a USB stick containing the files in
.DS C
.CW /public/firmware/intel/sr1520ml/testbios
.DE
and reset the machine.
Get into BIOS setup by pressing
.CW F2
repeatedly during the minute-long power-on self-test,
and select
.CW ->
.CW "Boot Manager"
.CW ->
.CW "EFI Shell"
.CW ->
.I newline .
Type
.P1
cd fs0:test_bios
fs0:
iflash32.efi /u /ni forMPtbl.Cap
.P2
Power cycle the machine when it prompts for a reset.
Get into BIOS setup and select
.CW ->
.CW Advanced
.CW ->
.CW "Processor Configuration"
.CW ->
.CW "enable vt for directed i/o"
and
.CW "multi-thread support in mps table" .
Save the changes and exit.
.
.SH
Coraid
.LP
Put your Coraid on a UPS.
Populate your Coraid with disks and configure it.
Pick a shelf number and decide how you want to split the drives into
logical units (if you don't want to just use all the disks as a single
raid 5 unit with spares).
We use shelf number 1, and
.I 9pcfs
is set up to expect that (see
.CW /sys/src/9/pc/boot.fs ).
Type this on your Coraid's keyboard (or serial console):
.P1
SR shelf unset> cecon /net/ether0
SR shelf unset> cecon /net/ether1
SR shelf unset> cecon /net/ether2
SR shelf unset> shelf 1
.P2
Add
.CW aoeif=
and
.CW aoedev=
lines to
.I plan9.ini
files on your servers; see
.I aoe (3),
.I sdaoe (3)
and
.I plan9.ini (8).
Our CPU servers use these to see the main partition on our Coraid
(which is shelf 1) as
.CW /dev/sde0
while booting (one can add other units later):
.P1
aoeif=ether1
aoedev=e!#\f(Jpæ\fP/aoe/1.0
.P2
The Coraid console can be reached via Ethernet using
.I cec (8).
You'll want to think about what other systems should share an Ethernet switch
with your Coraid.
Only those on the switch will be able to reach the
Coraid using AoE or cec, and this can provide useful isolation for
security since AoE and cec are unrouted Ethernet protocols.
.LP
Connect the Coraid PS/2 keyboard, mouse and video to the KVM switch.
.LP
Connect your Coraid and some existing Plan 9 system with a SATA
connector (it could be just a laptop with a local fossil) to an Ethernet switch.
This Plan 9 system needs
.CW fs ,
.CW aoe ,
and
.CW sdaoe
compiled into its kernel.
Connect to the Coraid console via
serial port, keyboard, or
.I cec (8),
and configure it as 2 spare drives, a
.CW raid5
lun 0 of 11 drives, a
.CW raidL
lun 1 of 1 drive, and a
.CW raidL
lun 2 of 1 drive.
Follow the Coraid manual to initialise the units;
type this on your Coraid's console:
.P1
SR shelf 1> spare 1.13-14
SR shelf 1> make 0 raid5 1.0-10		# this takes ~4 hours
SR shelf 1> make 1 raidl 1.11
SR shelf 1> make 2 raidl 1.12
SR shelf 1> online 0 1 2
.P2
Lun 0 is usable almost immediately; you need not wait for the raid 5
construction to finish.
.LP
Make
.CW /dev/sd[fg]0/data
visible per
.I sdaoe (3)
by typing on the Plan 9 system's console:
.P1
echo config switch on spec f type aoe//dev/aoe/1.1 >/dev/sdctl
echo config switch on spec g type aoe//dev/aoe/1.2 >/dev/sdctl
.P2
(See
.CW /rc/bin/addaoe
for a canned invocation.)
.
.SH
Partitions
.LP
Use
.I disk/prep
on Plan 9 to configure
.CW /dev/sd[efg]0/data
to match these partitions
(we don't bother with
.I fdisk
partitions because they can't hold large enough sector numbers):
.P1
; disk/prep /dev/sde0/data
  nvram                    0 1             (1 sectors, 512 B )
  empty                    1 10            (9 sectors, 4.50 KB)
  fscfg                   10 12            (2 sectors, 1.00 KB)
  empty                   12 14            (2 sectors, 1.00 KB)
  9fat                    14 204814        (204800 sectors, 100.00 MB)
  swap                204814 2097166       (1892352 sectors, 924.00 MB)
  fossil             2097166 8388622       (6291456 sectors, 3.00 GB)
  main               8388622 20971534      (12582912 sectors, 6.00 GB)
  sources           20971534 33554446      (12582912 sectors, 6.00 GB)
  bloom             33554446 34603022      (1048576 sectors, 512.00 MB)
  other             34603022 663748622     (629145600 sectors, 300.00 GB)
  outfs            663748622 670040078     (6291456 sectors, 3.00 GB)
  arena0           670040078 4881873405    (4211833327 sectors, 1.96 TB)
  empty           4881873405 4883970557    (2097152 sectors, 1.00 GB)
; disk/prep /dev/sdf0/data
  empty                   0 128          (128 sectors, 64.00 KB)
  isect                 128 241172608    (241172480 sectors, 115.00 GB)
  empty           241172608 488397053    (247224445 sectors, 117.88 GB)
; disk/prep /dev/sdg0/data
  empty                   0 128          (128 sectors, 64.00 KB)
  isect                 128 241172608    (241172480 sectors, 115.00 GB)
  empty           241172608 488397053    (247224445 sectors, 117.88 GB)
.P2
Populate
.CW /dev/sde0/fscfg
(which we use to configure
.CW /dev/fs )
with these contents:
.P1
; cat /dev/fs/ctl
cat main /dev/sde0/main
cat bloom /dev/sde0/bloom
cat fossil /dev/sde0/fossil
cat arena0 /dev/sde0/arena0
.P2
and configure your chosen Plan 9 system's
.CW /dev/fs
identically.
.LP
Initialise
.I 9fat .
.P1
disk/format -b /386/pbslba -d -r 2 /dev/sde0/9fat /386/9load
.P2
Initialise
.I nvram .
.P1
cp /cfg/example/nvram /dev/sde0/nvram
.P2
.
.SH
Venti
.LP
Initialise your
.I venti
store, specifying 1-gigabyte arenas.
.P1
venti/fmtarenas arena. /dev/sde0/arena0
venti/fmtisect isect0. /dev/sdf0/isect
venti/fmtisect isect1. /dev/sdg0/isect
venti/fmtbloom /dev/sde0/bloom
9fs pie
bind -a /n/pie/cfg /cfg
venti/fmtindex /cfg/pie/venti.conf
.\" venti/buildindex -b -M1G /cfg/pie/venti.conf	# NB: this can take hours
.P2
Connect your Blu-Ray drive to the chosen Plan 9 system's SATA connector.
Restore from Blu-Ray disc into your
.I venti
store:
.P1
cd /sys/lib/backup
sam funcs		# make sure backupinit sets correct values
restore 0 /dev/sdE1	# if sdE1 is your Blu-Ray drive
.P2
Follow directions and it will start populating
.I venti ,
starting with
the first blu-ray disc.
Repeat, replacing 0 with the number of the
first arena on each disc, until you've processed them all.
.LP
Your
.I venti
store should now contain the contents of the MH one as of a
few months ago.
Injecting the newer sealed arenas on the MH
.I venti
store into yours should bring yours up to date.
A command similar to
.DS C
.CW "rx pie venti/rdarena >x; venti/wrarena x"
.DE
is often used to do this;
see
.CW /sys/lib/backup/venti2venti
for an example.
.LP
Store the
.I venti
configuration in
.I venti .
.P1
ramfs
venti/conf -w /dev/sde0/arena0 /cfg/pie/venti.conf
.P2
Start your
.I venti
server.
.P1
venti=tcp!127.0.0.1!17034
venti/venti -c /dev/sde0/arena0
.P2
.
.SH
Fossils
.LP
Initialise your fossils.
For your main file system, use the last
nightly dump score from the last arena in your
.I venti
store.
Assuming that that
.I venti
score (without leading `vac:') is in
.CW $score :
.P1
fossil/flfmt -v $score /dev/sde0/fossil
fossil/flfmt /dev/sde0/other
fossil/flfmt -v $score /dev/sde0/main
.P2
Store the fossil configurations in the fossils.
.P1
fossil/conf -w /dev/sde0/fossil /cfg/pie/fossil.stand.conf
fossil/conf -w /dev/sde0/other  /cfg/pie/fossil.other.conf
fossil/conf -w /dev/sde0/main   /cfg/pie/fossil.main.conf
.P2
Start the fossil servers.
.P1
fossil/fossil -f /dev/sde0/fossil
fossil/fossil -f /dev/sde0/other
fossil/fossil -f /dev/sde0/main
.P2
Mount the fossils.
.P1
mount -cC /srv/fossil.stand.open /n/stand
mount -cC /srv/fossil.other.open /n/other
mount -cC /srv/fossil.main.open /n/main
.P2
The machine that is to be your main file server should have a
.CW plan9.ini
or
.CW /cfg/pxe
file that looks like this (replace
.I a.b.c.d
with the ip address of your
.I venti
server):
.P1
; cat 00151730ceb8
ether0=type=igbepcie
ether1=type=igbepcie
console=0
aoeif=ether1
# 1 is the shelf number
aoedev=e!#\f(Jpæ\fP/aoe/1.0
# *fakeintrs=1
*kernelpercent=10
bootfile=ether0!/386/9pcfs
# bootfile=ether0!/386/9pccpu
fscfg=/dev/sde0/fscfg
venti=tcp!a.b.c.d!17034
*pxeini=1
*bootppersist=ether0
*noe820print=1
nvram=#S/sde0/nvram
.P2
Your other CPU servers should have similar ones, but with only
.CW 9pccpu
(of the
.CW bootfile
clauses)
uncommented.
.
.SH
KVM
.LP
Set up your IP KVM.
Ours is an Austin-Hughes UIP-1602 with firmware revision 4.0.5.
I called the MH one
.CW kvmmh.cs.bell-labs.com
for now.
It's best reached by
.CW http
or
.CW https .
The administrative user is
.I super
with our usual
.I root
password.
Enable Java and pop-ups in your web browser and connect to the URL
.CW https://kvmmh.cs.bell-labs.com
(substituting the name of your KVM).
Once logged in, you select a system by typing
ScrollLock-ScrollLock-space (three keystrokes) and navigating the menu
system.
Up and down arrows move through the list of systems, pressing
RETURN switches to the highlit system.
An ESC character will back you out of menus.
The initial user name and password at power-on are both
.CW 00000000 .
.LP
Always use category
.B 5e
or better cables; the video seems to require it.
Using plain category 5 cables fails at least some of the time.
.
.SH
Serial
.LP
Set up your Perle serial concentrator and arrange
.I consolefs (4)
access.
The MH one is
.CW serialmh.cs.bell-labs.com.
One accesses serial ports on the Perle CS9000 server with
.P1
telnet -r net!serialmh!10001		# for line 1
.P2
but at most one simultaneous
.I telnet
connection per line is permitted,
so to allow sharing, one would normally use
.I consolefs
via
.I C :
.P1
C chips
.P2
.CW /cfg/fish/consoledb
has the mapping between host names and Perle line
numbers, for the curious.
This is arranged by
.CW /cfg/fish/startserial .
.CW /cfg/fish/restartserial
will kill all processes involved in processing
serial lines (and all
.I telnet
processes) and restart them.
This can be handy if connections to the serial concentrator get stuck.
There is an
.I admin
login with our usual Unix
.I root
password:
.P1
; telnet -r net!serialmh!10001
.sp 0.3v
serial.cs.bell
login: admin
password: \fIunix-pw\fP
^U
boot from: ether0!/386/9pccpu
achille (135.104.9.2!67): /386/9pccpu
894709+986216+378404=2259329
entry: 0xf0100020
.sp 0.3v
Plan 9
.P2
.
.SH
Power bars
.LP
Set up your power bars.
The upper MH power bar,
.I ibootbarhigh ,
has these systems on these outlets:
.P1
1	fish/chips
2	pie/bovril
3	—
4	boundary
5	—
6	—
7	—
8	—
.P2
The lower power bar,
.I ibootbarlow ,
has these systems on these outlets:
.P1
1	martha (sources)
2	haggis/neeps
3	mince/tatties
4	lookout
5	yon (ethel)
6	—
7	—
8	roku (diversiorum)
.P2
One connects to
.CW https://ibootbarhigh.cs.bell-labs.com
or
.CW https://ibootbarlow.cs.bell-labs.com ;
these are outside systems.
Login as user
.I admin
with our usual Unix
.I root
password.
Select check boxes of systems you want to power-cycle and click
.CW Cycle .
The documentation claims that rebooting and resetting the power bars
does not change the power to the outlets;
.LG
.B "this is a lie" !
.NL
One way to reset them is to press the recessed black
.CW reset
button on the front,
probably with a paper clip; this does bring back power bars that have
dropped off the network due to using their
.I telnet
interface,
and without cycling power.
However, upgrading the firmware reboots the power bar,
and that
.B does
cycle the power to the outlets; it also resets the
power bar's state to unconfigured, so you have to reconfigure it all
over again.
Firmware in MH has been upgraded from 1.20.154 to 1.4b.241.
Using the
.I telnet
interface makes the
.I ibootbars
drop off the network,
but in theory one can use the command
.P1
set outlet 1 name fish/chips
.P2
to name outlet 1
.CW fish/chips .
.P1
set upload enable yes
.P2
enables firmware upgrades.
.
.SH
Beyond
.LP
You should now be able to add terminals and CPU servers
in the usual way to bring up the rest of your cluster.
