.HTML "Plan 9 — Release Notes
.TL
Plan 9 From Bell Labs
.br
Release Notes
.br
July 2021
.AU
Plan 9 Foundation
.LP
.sp -.4i
.nf
.ce 1000
Copyright © 2021 Plan 9 Foundation
All Rights Reserved
.sp .2i
.fi
.
.de Xx
.br
.ne 4
.LP
\(bu
..
.
.LP
The current release of the Plan 9 operating system
contains many improvements and additions to the system
since the long-ago Fourth Edition release of 2002 from Bell Labs.
We now assume 21st-century hardware.
.Xx
.I fossil (4)
and
.I venti (8)
are now the production file server and block store, respectively,
and better documented.
.I Venti
has been greatly improved internally
and there is a new
.I libventi ,
though
.I fossil
still uses the old one.
Note that
.I fossil
is currently only safe up to 32TB
due to use of 32-bit integers to hold block numbers,
as are
.I venti
index sections.
.I kfs (4)
and
.I lnfs (4)
are deprecated and
the old standalone file server
and the IL protocol are gone.
.I cwfs (4)
provides the function of the old file server as a user-mode program,
plus the ability to communicate via TCP or other non-IL/IPv4 protocols.
.Xx
Network booting of PCs via PXE is routine.
The smaller PC kernels
.I 9pc "" (
and
.I 9pccpu )
can now be loaded without loading
.I pcboot
first.
Better compression with
.I lzip
(see
.I gzip (1))
and dynamic loading of missing
.CW /boot
executables allow kernels to be still smaller.
Other machines normally boot via BOOTP/DHCP and TFTP without PXE.
.Xx
The PC bootstrap programs are now variants of the PC kernel,
and use unmodified kernel device drivers, so they can load
from any supported device.
.Xx
The PC port now really assumes Pentium (1993) or newer processor(s).
386 and 486 code remains for reference, but will decay away.
Multiprocessors are also the norm now, rather than an exotic architecture.
Only ISA, PCI and PCI-Express buses are still supported;
PCMCIA and all subsequent variants are no longer supported.
.Xx
PCI message-signalled interrupts are now implemented on the PC,
which can be particularly helpful when the PC BIOS tables are wrong.
.Xx
ARM machines such as the Trimslice and 32-bit Raspberry Pis
are now supported, including multi-core machines.
Floating point emulation now includes VFP instructions in addition
to the old 7500 FPA instructions, so it should be easy to make
.CW 5l
.CW -f
the default.
One MIPS machine, the Mikrotik Routerboard 450G, is now supported.
.Xx
Compilers now exist for AMD64 and 64-bit PowerPC processors.
There is a work-in-progress ARM64 compiler
that can't yet compile the whole system,
though some programs can be compiled.
The ARM compiler can now profile programs and generate ELF executables.
The MIPS loader can now emit ELF64 executables.
The DEC Alpha, Sun/Oracle SPARC, and Motorola 68000 and 68020
architectures have been retired.
.Xx
Some 10 Gb/s Ethernet adapters are now supported, mainly Intel ones,
notably the X540 and X550 which run 10GBASE-T over Category 6a copper cables.
Note that these cards run hot and will shut down if they overheat,
so dual-port cards such as the X540-T2 are not
necessarily a better deal than single-port cards.
.Xx
Support for PCI-E Intel Gigabit Ethernet adapters now includes the
i21[089].
The i217 stubbornly refuses to transmit.
.Xx
IPv6 support is better (in part due to access to the Internet via native IPv6),
and includes tunnelling through IPv4 to reach the wider IPv6 internet;
see
.I 6in4 (8).
Hanging bugs in ICMPv6, found through native IPv6 use, have been fixed, and
.I dial (2)
now dials all addresses (v4 and v6) concurrently until it connects.
.Xx
USB support has improved and now includes USB 2 (EHCI).
There are drivers for USB Ethernet dongles and the like;
see
.I usb (3)
and
.I usb (4).
USB 3 (XHCI) controllers can be recognised and disabled
to reduce their interrupt load.
.Xx
The cryptographic infrastructure is being updated.
SHA2 is now available; see
.I sechash (2).
Use of RC4 and SSLv2 are being deprecated but will take a while to eliminate
due to backward compatibility.
In particular,
.I secstore (1)
and
.I factotum (4)
have use of SSLv2 hard-wired into their protocols.
.I ssh1 (1)
no longer uses RC4.
.I tls (3)
and
.I tls (2)
can now cope with SSLv3 and TLS 1.[0-2].
.I cpu (1)
and
.I drawterm (8)
now attempt TLS connections and may fall back to SSLv2.
.I import (1)
and
.I exportfs (8)
now implement TLS encryption.
.I import
used to default to no encryption,
but now defaults to trying TLS, SSLv2 and then clear text,
with a warning if TLS can't be used.
Note that currently clear-text imports must be used if it is necessary to
mount an imported
.I srv (3)
channel;
this is being investigated.
.Xx
Related to the cryptographic upgrade,
support for 9P1 (the 15-year-old 9P) is gradually being dropped;
everything should be converted to 9P2000 by now.
.Xx
There is now native support for SATA devices
(with block addresses up to 48 bits wide)
via AHCI,
and for NVMe solid-state disks.
.Xx
.I Cdfs
now supports access to data (not audio nor video)
on DVDs and Blu-ray discs.
.Xx
VESA video on PCs now works fairly reliably, even on multiprocessors.
.Xx
ATA-over-Ethernet is supported;
see
.I aoe (3).
iSCSI is also supported.
.Xx
Unicode Runes are now 21 bits wide and UTF sequences may be 4 bytes long.
.Xx
Restarting the system without a hardware reset now works well;
see
.I fshalt (8).
.Xx
.I ndb/dns
has been extended and improved in various ways but
its UDP server still has hard-to-find memory leaks and eventually corruption.
We can only recommend dedicating machines to act as UDP nameservers and
rebooting them frequently.
.Xx
.I "virtual machines."
Recent releases of Virtualbox (e.g., 5.2.26)
will run Plan 9, though slowly.
If you want to try anyway, in Settings, in the System & Acceleration tabs,
choose `\c
.CW Paravirtualization
.CW Interface
.CW Hyper-V '.
Recent releases of Parallels (e.g., 13.3.0)
and Vmware Esxi 6.7
can be configured to run Plan 9:
choose the Intel Gigabit CT Ethernet controller.
Parallels is slower, with higher network latency.
Imaginary hardware is of less importance than running well on real hardware,
and the increased Ethernet latency in most virtual machines hosted on
general-purpose operating systems is appalling.
.Xx
.I "64-bit kernels."
We are now also distributing a 64-bit-capable kernel,
.CW /sys/src/9k ,
which has two ports, to the 64-bit
.CW amd64
and
.CW riscv64
architectures,
intended for use as a cpu or file server;
VGA graphics, USB, etc. have been omitted to date.
The late Jim McKie
(\c
.I jmk )
created
.CW 9k
from our 32-bit kernel.
It implements the same system calls, and some of the same devices,
with an emphasis on PCI-Express devices.
.Xx
The
.CW riscv64
port currently runs on the Microchip Polarfire Icicle,
pre-release Beagle V,
and HiFive Unmatched.
Richard Miller contributed the RISC-V compiler suites.
.Xx
The
.CW 9
and
.CW 9k
kernels have diverged somewhat, but we will be attemping to minimise
source differences.
In particular, the
.CW boot
directories are identical,
and the
.CW ip
directories are almost identical,
but some device drivers differ slightly between the two kernels.
ACPI is almost entirely ignored and use of MSI avoids most BIOS table
(e.g., MPS) bugs.
Note that the
.I malloc (2)
and
.I pool (2)
allocators now use
.I uintptr
variables internally and in their interfaces,
and so can cope with 64-bit addresses,
permitting allocating huge tracts of memory.
.Xx
.I Regfree
(see
.I regexp (2))
now exists and should be used instead of
.I free
to free compiled regular expressions.
.LP
The Plan 9 Wiki
.CW http://plan9foundation.org/wiki/plan9 ) (
is the place to visit to learn more and stay current.
The Usenet group
.CW comp.os.plan9 ) (
used to a good place to visit,
but it has degenerated.
.LP
Good Luck!
