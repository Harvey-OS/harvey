.HTML "Bootstrapping Plan 9 on PCs
.de Os\" overstrike argument
\\$1\l'|0–'
..
.
.TL
Bootstrapping Plan 9 on PCs
.AU
Geoff Collyer
.br
.CW geoff@plan9.bell-labs.com \(dg
.AI
.MH
.AB
What's interesting, tricky or non-obvious
about bootstrapping Plan 9 on PCs?
.AE
.
.FS
\(dg now
.CW geoff@collyer.net
.FE
.
.SH
Introduction
.LP
Plan 9 has new PC bootstraps,
.I 9boot
and
.I 9load ,
replacing the decade-old
.I 9pxeload
and
.I 9load
programs.
What did we learn while writing them?
.SH
PC Constraints
.LP
The IBM PC imposes quite a few constraints on bootstrap programs
(programs that load operating system kernels).
.LP
A PC starts executing in 16-bit `real' (Intel 8086) mode
and has no boot monitor, as other machines do,
just a primitive yet huge BIOS that will perform a power-on self-test (POST)
and attempt to read boot sectors
from disks or load a modest payload from the network via TFTP.
(Actually some new machines have much more complicated and dangerous
boot loaders called (U)EFI, but we don't deal with EFI.)
The boot sectors must load further bootstrap programs
that resemble the TFTP payload.
These bootstrap programs can only address the first megabyte of memory
until they get out of real mode,
and even then the upper 384KB of the initial megabyte is reserved
for BIOS and device ROMs.
This limits boot programs to at most 640K,
and less for ones loaded by PXE (network boot) ROMs.
.LP
The ceiling on memory available to PXE payloads is unclear.
Various sources give it as 480K (UNDI start; Symantec),
536K (Intel Nuc),
564K (base of stack; PXE spec.),
606K (PXE spec.),
or
624K (PC Engines APU2),
or the start of the EBDA, the extended BIOS data area.
The top of `free base memory' is a likely ceiling
and is
.CW "*(ushort*)0x413"
(sic),
expressed in kilobytes.
The address of the EBDA is
.CW "*(ushort*)0x40e << 4" .
It is often 639K, but has been seen as low as 626K on an Intel Nuc,
and could be even lower.
The PXE ROMs are likely to enforce the payload ceiling,
but we must be careful not to step on the EBDA.
As a result of the payload ceiling not being fixed, it's not obvious
how large a program we can load via PXE.
.LP
BIOS calls (via the
.CW INT
instruction)
only work in real mode,
so the bootstraps execute BIOS calls to learn the machine's
memory map and power management configuration,
and stash the results in the first megabyte
for later retrieval by the loaded kernel.
Empirically, some BIOSes enable interrupts (with
.CW STI
instructions)
during BIOS calls,
so the bootstraps disable them again after each call;
failure to do so often results in an interrupt,
perhaps from the clock, resetting the machine.
.I 9loadusb
returns briefly to real mode
to read USB devices and has mixed results with that.
.LP
Getting into 32-bit protected mode permits addressing the first 4GB of memory,
but first it is necessary to enable the A20 address line (the
.CW 1<<20
bit).
For (extreme) backward compatibility, this bit is normally held to zero
until software requests that it be released, and holding it to zero will cause
references to the second megabyte to be mapped to the first, etc.,
causing bizarre-seeming memory corruption.
The old technique was to ask the keyboard controller to release it,
but some systems now have no keyboard controller (they are servers
or have USB keyboards).
We have found it necessary to keep trying different methods until one
is verified to have succeeded.
The new bootstraps also try an
.CW INT
.CW 15
BIOS call and
manipulation of port
.CW 0x92
(`system control' on some systems).
.LP
Even in protected mode with A20 enabled, some systems
force a gap in the physical address space between 15MB and 16MB,
which must be avoided.
.
.SH
Plan 9 Requirements
.IP • 3
The new bootstraps must be able to load 64-bit
.CW amd64
kernels as well as 32-bit
.CW 386
ones.
In addition to Plan 9 boot image format,
the new bootstraps understand ELF and ELF64 formats.
.IP •
Plan 9 kernels need to be started in 32-bit protected mode and
implicitly assume that A20 is enabled.
.IP •
They expect a parsed
.CW /cfg/pxe/\fIether
or
.CW plan9.ini
file to be present at physical address
.CW 0x1200 .
.IP •
They expect that
.I 9load
will have added entries to the parsed
.CW plan9.ini
describing any disk partitions found.
.B NB:
.I 9boot
does not do this and so kernels loaded by it
that use disk partitions before
.I cpurc
or
.I termrc
runs will need to
.I not
have
.CW readparts=0
in
.CW plan9.ini ,
if any.
.IP •
They expect the E820 memory map and
automatic power management information obtained from the
BIOS to be present in the first megabyte.
.IP •
Our
.CW 9k
.CW amd64
kernels also expect a Gnu
.I multiboot
header containing any arguments and a memory map.
.
.SH
Non-Requirements
.IP • 3
The bootstraps should ignore secondary processors, leaving them in reset.
.IP •
The bootstraps need not do anything with floating point.
.
.SH
Techniques and Tricks
.LP
Our new bootstraps are stripped-down Plan 9 PC kernels
without system calls and user-mode processes.
They share the vast majority of their code with the ordinary PC kernels.
750 lines of C are changed files from the PC kernel,
another 7,800 lines are new or carried forward (possibly modified)
from the old
.I 9load .
In particular, they use the ordinary PC kernel's device drivers,
unmodified.
.I 9boot
loads kernels via TTFTP\(dg
.FS
\(dg Here's the entire protocol:
write a line with a file's path name to TCP port 17015 on your file server,
then read its contents until the file server closes the connection.
.FE
(or can be configured to use PXE);
.I 9load
loads kernels from disk.
This is more specialised than the old all-in-one
.I 9load ,
in order to fit into low memory.
.LP
From protected mode,
the bootstraps initially enable paging for their own use.
Before jumping to the loaded kernel, they turn paging off, reverting
to plain protected mode, providing a known initial CPU state
for the kernels.
.LP
Self-decompression of the bootstraps
helps to relieve the ~600KB payload limits
on PXE-loaded bootstraps and PBS-loaded ones.
Our version of
Russ Cox's decompressing header* code is about 12K all told,
.FS
* see
.CW http://plan9.bell-labs.com/wiki/plan9/Replacing_9load
.FE
including BIOS calls to get APM and E820 memory map info.
It now also converts that E820 map to a Multiboot map,
for the benefit of our
.I amd64
kernels loaded directly by PXE.
We initially compressed only
.I 9boot
(because it was approaching 600K with a full complement of Ethernet drivers)
but now compress all the bootstraps.
Stripping the kernel and switching from
.I gzip
to
.I lzip
compression gives a typical size-reduction ratio of 4:1,
which makes feasible direct loading (e.g., by PXE and bypassing
.I 9boot
or
.I 9load )
of compressed kernels that fit in free base memory,
which includes generic terminal and CPU kernels.
.LP
The bootstraps also will decompress
.I lzip -ped
kernels loaded from disk,
mainly for CD or other emulated-floppy booting,
where there are limits on kernel size.
.LP
Figure 1 shows the memory map in effect while the bootstraps run.
.KF
.TS
center ;
cb s ,
n cw(4.5i) .
Figure 1: Map of physical memory during bootstrapping
.sp 0.3v
0	interrupt vectors
_
1K	BIOS data area
_
1¼K	T{
various uses per \fImem.h\fP, notably E820 and multiboot memory maps;
MBR relocates itself to 1½K
T}
_
31K	T{
pbs loaded here.
also start of pxe-loaded decompressor + compressed bootstrap.
decompresses self to 9MB.
T}
_
64K	T{
start of pbs-loaded decompressor + compressed bootstrap.
decompresses self to 9MB.
T}
\&...
_
~512K	first pxe loader, from ROM
_
~626K	extended BIOS data area (EBDA)
_
640K	UMB: option ROMs
_
896K	UMB: BIOS ROMs
_
1M	ultimate plan 9 kernel destination
_
9M	bootstrap after decompression.
_
13M	T{
optional plan 9
\fIkernel\fP\fB.lz\fP
(maximum 2MB).
.I 9load
will load and decompress this to a kernel at 1M.
T}
_
15M	historical no-man's land (ISA bus hole)
_
16M	malloc arena for 9boot
\&...
.TE
.KE
.LP
Our USB stack (at least 3 HCI drivers plus user-mode drivers,
implying system calls and user-mode support) is too big
to fit in the first 640KB (or less),
so the bootstraps try to get BIOSes to read from USB devices
and some of them do.
Real mode BIOS calls are an ugly solution,
but USB is an ugly problem.
.LP
We strongly prefer network booting; disk booting is a poor second.
Network booting minimises the number of copies of kernels that must
be updated and ensures that machines boot the latest kernels.
.
.SH
Other differences from the old bootstraps
.LP
The old
.I 9load
was a single process derived (but separately evolved) from
an old PC kernel, so it needed modified device drivers
(primarily for ethernet and disk controllers), which was an ongoing
maintenance nuisance, and not all ethernet controllers had bootstrap drivers.
The new bootstraps are just specialised Plan 9 kernels that
are loaded compressed into the first 640KB of RAM,
so they use unmodified Plan 9 drivers.
There isn't enough room below 640K to include useful support
for user mode and system calls, but the new bootstraps do implement
kernel processes.
.LP
The old
.I 9load
was capable of loading from disks, floppies, USB disks or ethernet (via PXE).
Again due to space limitations,
we've had to focus the new bootstraps more sharply.
.I 9boot
loads via TTFTP (or PXE) and nothing else, which is our usual mode of operation.
.I 9load
loads from non-USB disks but not floppies,
which ought to be obsolete by now.
If you must boot from floppy and are doing so now,
and your BIOS won't boot from USB,
do this to arrange to boot from USB disk instead:
.DS
.ft CW
a: && cp /386/9loadusb /n/a:/9load
.ft
.DE
.I 9loadusb
loads from USB disks only (using BIOS INT 13 calls).
BIOSes seem to be easily confused by intermixing direct I/O and BIOS calls,
thus we keep
.I 9load
and
.I 9loadusb
distinct.
.LP
The new
.I 9load
will look on all available disks for FAT file systems.
If no bootfile is specified in a
.CW plan9.ini ,
it will examine each file
system to see if it contains a single Plan 9 kernel
(\c
.CW 9pc*
or
.CW 9k10* ),
and if so, will boot it.
.LP
.I 9boot
contains no disk drivers, so it can't read partition
tables and populate
.CW #ec/sd??part
for the kernel's benefit, so if you need to access a disk partition
early in the kernel's execution
(e.g., you have an nvram partition),
you'll want to ensure that
.DS
.ft CW
readparts=0
.ft
.DE
is
.I not
present in the
.CW /cfg/pxe
file for any such machines.
.LP
The new bootstraps run on more machines than the old ones did.
We discovered new ways to enable the A20 address line and try them all
until success.
This may fix various odd memory corruption problems
seen in the past.
We also discovered that BIOS calls may enable
interrupts, so we disable them again immediately upon return.
This
may prevent mysterious resets seen with the old bootstraps.
.SH
CD booting changes
.LP
.I Pbsraw
can be up to 2KB, so we can print and provide better service;
it uses things written by
.I mk9660 .
It reads a contiguous file and is 468 bytes long
(we dropped the
.CW 9fat
support),
so it could be used to load from any raw partition supporting LBA access.
.LP
There are corresponding changes to
.I mk9660
to annotate the PBS.
A new parameter,
.CW -x
.I loader ,
names a loader,
which must be in the root directory.
In conjunction with
.CW -B ,
it can be used to boot directly from the CD.
.LP
.I 9load
was changed to read a
.CW 9fat
image file in the root directory of a CD image,
which must be called
.CW bootdisk.img
and can be of any size but must be contiguous.
.br
.ne 6
.SH
Future Horrors/Directions
.LP
We haven't dealt at all with (U)EFI, `secure boot', GPTs nor GUIDs.
Current real-mode operations,
such as obtaining the physical memory map
and power management information, and
resetting VGA and floppy controllers,
would have to be done with UEFI calls in protected mode
rather than real-mode BIOS calls.
We
.I can
use bare Plan 9 partition tables instead of MBRs or GPTs
to address disks larger than 2 TB.
.
.SH
Lessons Learned
.LP
A disabled A20 line can masquerade as all sorts of baffling problems.
It is well worth ensuring that it is truly enabled.
.LP
Real-mode code is hard to get right,
even when you know that it is hard to get right.
Conditional jumps only take byte displacements, so for targets
farther away, we must use an unconditional long jump.
.LP
Virtual-machine hypervisors can be good test-beds and provide
better crash diagnostics than the
blank screen you get on real hardware,
but they can also mislead
(e.g.,
.CW amd64
kernels on Virtualbox,
Vmware 7 on Ubuntu 12.04).
.LP
All of these bootstrap programs and the BIOS (and POST) can be avoided,
once Plan 9 is running,
by using
.CW /dev/reboot
as packaged up in
.I fshalt (8),
which is much faster.
.LP
Compressed kernels that fit in free base memory
.I 9pclz "" (
and
.I 9pccpulz )
can now be directly loaded via PXE and bypass these bootstraps.
.SH
Acknowledgements
.LP
Jim McKie
wrote previous PC bootstraps, notably the original
.I 9load
and its DOS predecessors,
.I l.com
and
.I b.com ,
from which much code has been borrowed.
Russ Cox
described the decompressing header used by these bootstraps.
