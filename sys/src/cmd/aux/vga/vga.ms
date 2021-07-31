.TL
Plan 9 VGA support
.AU
Bill Cheswick
.SH 1
Overview
.PP
When Plan 9 boots on a PC, it comes up in CGA text mode.  
The kernel can initialize and support a simple, standard 640x480 VGA
display:
.P1
	echo 640x480x1 > '#v/vgasize'
.P2
This should work on any VGA card.  Other higher resolution configurations
are initialized by
.FR aux/vga ,
which reads configuration information from
.FR /lib/vga .
In these modes, the kernel must contain support for bank-switching
the VGA display memory.
.PP
Computing the VGA registers for a particular VGA board, SVGA chip,
and monitor takes a lot of work.  In DOS, this work is performed by
the ROM on the VGA card.  The ROM code is not available to Plan 9, so
we do the following:
.IP 1
Use the DOS INT 0x10 call to set the mode we want.
.IP 2
Dump the standard VGA registers, plus whatever extended registers this
particular chip needs.
.IP 3
Add the register values to the configuration file
.FR /lib/vga .
.IP 4
Run
.F aux/vga
to load the registers and configure the kernel.
.SH 1
Kernel support
.PP
The kernel VGA driver supports the following files:
.P1
#v/vgasize
#v/vgatype
#v/vgaport
.P2
