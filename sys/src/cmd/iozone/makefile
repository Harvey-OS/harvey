#
# Version $Revision: 1.144 $
#
# The makefile for building all versions of iozone for all supported
# platforms
#
# Supports:	hpux, hpux_no_ansi, hpux-10.1, hpux_no_ansi-10.1,
#		sppux, sppux-10.1, ghpux, sppux, 
#		convex, FreeBSD, OpenBSD, OSFV3, OSFV4, OSFV5, SCO
#		SCO_Unixware_gcc,NetBSD,TRU64, Mac OS X

CC	= cc
C89	= c89
GCC	= gcc
CCS	= /usr/ccs/bin/cc
NACC	= /opt/ansic/bin/cc
CFLAGS	=
S10GCCFLAGS    = -m64
S10CCFLAGS     = -m64
FLAG64BIT      = -m64

# If your Linux kernel supports preadv and pwritev system calls 
# and you want iozone to use them, add -DHAVE_PREADV -DHAVE_PWRITEV
# to CFLAGS

all:  
	@echo ""
	@echo "You must specify the target.        "
	@echo "        ->   AIX                  (32bit)   <-"
	@echo "        ->   AIX-LF               (32bit)   <-"
	@echo "        ->   AIX64                (32bit)   <-"
	@echo "        ->   AIX64-LF             (32bit)   <-"
	@echo "        ->   bsdi                 (32bit)   <-" 
	@echo "        ->   convex               (32bit)   <-" 
	@echo "        ->   CrayX1               (32bit)   <-"
	@echo "        ->   dragonfly            (32bit)   <-"
	@echo "        ->   freebsd              (32bit)   <-"
	@echo "        ->   generic              (32bit)   <-"
	@echo "        ->   ghpux                (32bit)   <-"
	@echo "        ->   hpuxs-11.0 (simple)  (32bit)   <-"
	@echo "        ->   hpux-11.0w           (64bit)   <-"
	@echo "        ->   hpuxs-11.0w          (64bit)   <-"
	@echo "        ->   hpux-11.0            (32bit)   <-"
	@echo "        ->   hpux-10.1            (32bit)   <-"
	@echo "        ->   hpux-10.20           (32bit)   <-"
	@echo "        ->   hpux                 (32bit)   <-" 
	@echo "        ->   hpux_no_ansi         (32bit)   <-"
	@echo "        ->   hpux_no_ansi-10.1    (32bit)   <-"
	@echo "        ->   IRIX                 (32bit)   <-"
	@echo "        ->   IRIX64               (64bit)   <-"
	@echo "        ->   linux                (32bit)   <-"
	@echo "        ->   linux-arm            (32bit)   <-"
	@echo "        ->   linux-AMD64          (64bit)   <-"
	@echo "        ->   linux-ia64           (64bit)   <-"
	@echo "        ->   linux-powerpc        (32bit)   <-"
	@echo "        ->   linux-powerpc64      (64bit)   <-"
	@echo "        ->   linux-sparc          (32bit)   <-"
	@echo "        ->   macosx               (32bit)   <-"
	@echo "        ->   netbsd               (32bit)   <-"
	@echo "        ->   openbsd              (32bit)   <-"
	@echo "        ->   openbsd-threads      (32bit)   <-"
	@echo "        ->   OSFV3                (64bit)   <-"
	@echo "        ->   OSFV4                (64bit)   <-"
	@echo "        ->   OSFV5                (64bit)   <-"
	@echo "        ->   linux-S390           (32bit)   <-"
	@echo "        ->   linux-S390X          (64bit)   <-"
	@echo "        ->   SCO                  (32bit)   <-"
	@echo "        ->   SCO_Unixware_gcc     (32bit)   <-"
	@echo "        ->   Solaris              (32bit)   <-"
	@echo "        ->   Solaris-2.6          (32bit)   <-"
	@echo "        ->   Solaris7gcc          (32bit)   <-"
	@echo "        ->   Solaris8-64          (64bit)   <-"
	@echo "        ->   Solaris8-64-VXFS     (64bit)   <-"
	@echo "        ->   Solaris10            (32bit)   <-"
	@echo "        ->   Solaris10cc          (64bit)   <-"
	@echo "        ->   Solaris10gcc         (32bit)   <-"
	@echo "        ->   Solaris10gcc-64      (64bit)   <-"
	@echo "        ->   sppux                (32bit)   <-"
	@echo "        ->   sppux-10.1           (32bit)   <-"
	@echo "        ->   sppux_no_ansi-10.1   (32bit)   <-"
	@echo "        ->   SUA                  (32bit)   <-"
	@echo "        ->   TRU64                (64bit)   <-"
	@echo "        ->   UWIN                 (32bit)   <-"
	@echo "        ->   Windows (95/98/NT)   (32bit)   <-"
	@echo ""

clean:
	rm -f *.o iozone fileop pit_server

rpm:
	cp ../../iozone*.tar /usr/src/red*/SO*
	rpmbuild -ba spec.in


#
# Turn on the optimizer, largefiles, Posix async I/O and threads.
#
hpux-11.0:	iozone_hpux-11.0.o libasync.o libbif.o
	 $(CC) +O3 +Oparallel $(LDFLAGS) iozone_hpux-11.0.o libasync.o \
		libbif.o -lpthread -lrt -o iozone

#
# Turn on wide-mode, the optimizer, largefiles, Posix async I/O and threads.
#
hpux-11.0w:	iozone_hpux-11.0w.o libasyncw.o libbif.o
	 $(CC) +DD64 +O3 $(LDFLAGS) iozone_hpux-11.0w.o libasyncw.o \
		libbif.o -lpthread -lrt -o iozone


#
# Simple build with largefiles, Posix threads and Posix async I/O
#
hpuxs-11.0:	iozone_hpuxs-11.0.o libasync.o libbif.o fileop_hpuxs-11.0.o pit_server.o
	 $(CC) -O $(LDFLAGS)  iozone_hpuxs-11.0.o \
		libasync.o libbif.o -lpthread -lrt -o iozone
	 $(CC) -O $(LDFLAGS) fileop_hpuxs-11.0.o -o fileop
	 $(CC) -O $(LDFLAGS) pit_server.o -o pit_server

#
# Simple build with wide-mode, largefiles, Posix threads and Posix async I/O
#
hpuxs-11.0w:	iozone_hpuxs-11.0w.o libasyncw.o libbif.o
	 $(CC) -O +DD64 $(LDFLAGS) iozone_hpuxs-11.0w.o \
		libasyncw.o libbif.o -lpthread -lrt -o iozone

#
# Simple 10.1 build with no threads, no largefiles, no async I/O 
#
hpux-10.1:	iozone_hpux-10.1.o  libbif.o
	 $(C89) +e -O $(LDFLAGS) iozone_hpux-10.1.o libbif.o -o iozone

hpux-10.20:	iozone_hpux-10.20.o  libbif.o
	 $(C89) +e -O $(LDFLAGS) iozone_hpux-10.20.o libbif.o -o iozone

#
# Simple generic HP build with no threads, no largefiles, no async I/O 
#
hpux:	iozone_hpux.o 
	$(C89) +e -O $(LDFLAGS) iozone_hpux.o libbif.o -o iozone

#
# GNU HP build with no threads, no largefiles, no async I/O 
#
ghpux:	iozone_ghpux.o  libbif.o
	$(GCC) -O $(LDFLAGS) iozone_ghpux.o libbif.o -static -o iozone

#
# GNU Generic build with no threads, no largefiles, no async I/O 
#
generic:	iozone_generic.o  libbif.o
	$(CC)  -O $(LDFLAGS) iozone_generic.o libbif.o -o iozone

#
# No ansii 'C' compiler HP build with no threads, no largefiles, no async I/O 
#
hpux_no_ansi-10.1:	iozone_hpux_no-10.1.o  libbif.o 
	$(NACC)  -O $(LDFLAGS) iozone_hpux_no-10.1.o libbif.o -o iozone

#
# No ansii 'C' compiler HP build with no threads, no largefiles, no async I/O 
#
hpux_no_ansi:	iozone_hpux_no.o  libbif.o
	$(C89)  -O $(LDFLAGS) iozone_hpux_no.o libbif.o -o iozone

#
# GNU 'C' compiler Linux build with threads, largefiles, async I/O 
#
linux:	iozone_linux.o libasync.o libbif.o fileop_linux.o pit_server.o
	$(CC)  -O3 $(LDFLAGS) iozone_linux.o libasync.o libbif.o -lpthread \
		-lrt -o iozone
	$(CC)  -O3 -Dlinux fileop_linux.o -o fileop
	$(CC)  -O3 -Dlinux pit_server.o -o pit_server

#
# GNU 'C' compiler Linux build for powerpc chip with threads, largefiles, async I/O 
#
linux-powerpc: iozone_linux-powerpc.o  libbif.o libasync.o fileop_linux-ppc.o pit_server.o
	$(CC) -O3 $(LDFLAGS) iozone_linux-powerpc.o libasync.o \
		libbif.o -lpthread  -lrt -o iozone
	$(CC)  -O3 -Dlinux fileop_linux-ppc.o -o fileop
	$(CC)  -O3 -Dlinux pit_server.o -o pit_server
#
# GNU 'C' compiler Linux build for sparc chip with threads, largefiles, async I/O 
#
linux-sparc: iozone_linux-sparc.o  libbif.o libasync.o fileop_linux.o pit_server.o
	$(CC) -O3 $(LDFLAGS) iozone_linux-sparc.o libasync.o libbif.o \
		-lpthread -lrt -o iozone
	$(CC) -O3 -Dlinux fileop_linux.o -o fileop
	$(CC) -O3 -Dlinux pit_server.o -o pit_server

#
# GNU 'C' compiler Linux build with threads, largefiles, async I/O 
#
linux-ia64:	iozone_linux-ia64.o  libbif.o libasync.o fileop_linux-ia64.o pit_server.o
	$(CC) -O3 $(LDFLAGS) iozone_linux-ia64.o libbif.o libasync.o \
		-lrt -lpthread -o iozone
	$(CC)  -O3 -Dlinux fileop_linux-ia64.o -o fileop
	$(CC)  -O3 -Dlinux pit_server.o -o pit_server

#
# GNU 'C' compiler Linux build for powerpc chip with threads, largefiles, async I/O 
#
linux-powerpc64: iozone_linux-powerpc64.o  libbif.o libasync.o fileop_linux-ppc64.o pit_server-linux-powerpc64.o
	$(CC) -O3 -Dunix -DHAVE_ANSIC_C -DSHARED_MEM -DASYNC_IO \
		-D_LARGEFILE64_SOURCE -Dlinux \
		iozone_linux-powerpc64.o libasync.o libbif.o -lpthread \
		-lrt $(FLAG64BIT) -o iozone
	$(CC)  -O3 -Dlinux fileop_linux-ppc64.o $(FLAG64BIT) -o fileop
	$(CC)  -O3 -Dlinux pit_server-linux-powerpc64.o $(FLAG64BIT) -o pit_server
		
#
# GNU 'C' compiler Linux build with threads, largefiles, async I/O
#
linux-arm:	iozone_linux-arm.o  libbif.o libasync.o fileop_linux-arm.o pit_server.o
	$(CC) -O3 $(LDFLAGS) iozone_linux-arm.o libbif.o libasync.o \
		-lrt -lpthread -o iozone
	$(CC) -O3 -Dlinux fileop_linux-arm.o -o fileop
	$(CC) -O3 -Dlinux pit_server.o -o pit_server

#
# GNU 'C' compiler Linux build with threads, largefiles, async I/O 
#
linux-AMD64:	iozone_linux-AMD64.o  libbif.o libasync.o fileop_linux-AMD64.o pit_server.o
	$(CC)  -O3 $(LDFLAGS) iozone_linux-AMD64.o libbif.o libasync.o \
		-lrt -lpthread -o iozone
	$(CC)  -O3 -Dlinux fileop_linux-AMD64.o -o fileop
	$(CC)  -O3 -Dlinux pit_server.o -o pit_server

#
# GNU 'C' compiler Linux build with S/390, threads, largfiles, async I/O
#
linux-S390:	iozone_linux-s390.o libbif.o libasync.o fileop_linux-s390.o pit_server.o
	$(CC)  -O2 $(LDFLAGS) -lpthread -lrt iozone_linux-s390.o \
		libbif.o libasync.o -o iozone
	$(CC)  -O3 -Dlinux fileop_linux-s390.o -o fileop
	$(CC)  -O3 -Dlinux pit_server.o -o pit_server

#
# GNU 'C' compiler Linux build with S/390, threads, largfiles, async I/O
#
linux-S390X:	iozone_linux-s390x.o libbif.o libasync.o fileop_linux-s390x.o pit_server.o
	$(CC)  -O2 $(LDFLAGS) -lpthread -lrt iozone_linux-s390x.o \
		libbif.o libasync.o -o iozone
	$(CC)  -O3 -Dlinux fileop_linux-s390x.o -o fileop
	$(CC)  -O3 -Dlinux pit_server.o -o pit_server


# 
# AIX
# I would have built with ASYNC_IO but the AIX machine does not have 
# POSIX 1003.1b compliant async I/O header files.  Has threads, no
# largefile support.
# 
AIX:	iozone_AIX.o  libbif.o  fileop_AIX.o
	$(CC)  -O $(LDFLAGS) iozone_AIX.o libbif.o \
		-lpthreads -o iozone
	$(CC)  -O -Dlinux fileop_AIX.o -o fileop

# 
# AIX-LF
# I would have built with ASYNC_IO but the AIX machine does not have 
# POSIX 1003.1b compliant async I/O header files.  Has threads, and
# largefile support.
# 
AIX-LF:	iozone_AIX-LF.o  libbif.o   fileop_AIX-LF.o pit_server.o
	$(CC)  -O $(LDFLAGS) iozone_AIX-LF.o libbif.o \
		-lpthreads -o iozone
	$(CC)  -O fileop_AIX-LF.o -o fileop
	$(CC)  -O pit_server.o -o pit_server

# AIX64
# This version uses the 64 bit interfaces and is compiled as 64 bit code.
# Has threads, async I/O but no largefile support.
#
AIX64:        iozone_AIX64.o libbif.o fileop_AIX64.o libasync.o pit_server.o
	$(GCC) -maix64 -O3 $(LDFLAGS) iozone_AIX64.o libasync.o \
              libbif.o -lpthreads -o iozone
	$(GCC) -maix64 -O3 $(LDFLAGS) -Dlinux fileop_AIX64.o -o fileop
	$(GCC) -maix32 -O3 $(LDFLAGS) pit_server.o -o pit_server

#
# AIX64-LF
# This version uses the 64 bit interfaces and is compiled as 64 bit code.
# Has threads, async I/O and largefile support.
#
AIX64-LF:     iozone_AIX64-LF.o libbif.o fileop_AIX64-LF.o libasync.o pit_server.o
	$(GCC) -maix64 -O3 $(LDFLAGS) iozone_AIX64-LF.o libasync.o \
              libbif.o -lpthreads -o iozone
	$(GCC) -maix64 -O3 $(LDFLAGS) -Dlinux fileop_AIX64-LF.o -o fileop
	$(GCC) -maix32 -O3 $(LDFLAGS) pit_server.o -o pit_server

#
# IRIX 32 bit build with threads, largefiles, async I/O 
# This would like to be in 64 bit mode but it hangs whenever in 64 bit mode.
# This version uses the 64 bit interfaces but is compiled as 32 bit code
#
IRIX64:	iozone_IRIX64.o libasyncw.o libbif.o 
	$(CC)   -32 -O $(LDFLAGS) iozone_IRIX64.o libbif.o \
		-lpthread libasyncw.o -o iozone

#
# IRIX 32 bit build with threads, No largefiles, and async I/O 
# This version uses the 32 bit interfaces and is compiled as 32 bit code
#
IRIX:	iozone_IRIX.o libasync.o libbif.o
	$(CC)  -O  -32 $(LDFLAGS) iozone_IRIX.o libbif.o -lpthread \
		libasync.o -o iozone

#
# CrayX1: 32 bit build with threads, No largefiles, and async I/O 
# This version uses the 32 bit interfaces and is compiled as 32 bit code
#
CrayX1:	iozone_CrayX1.o libasync.o libbif.o
	$(CC)  -O  $(LDFLAGS) iozone_CrayX1.o libbif.o \
		-lpthread libasyncw.o -o iozone

#
# SPP-UX 32 bit build with threads, No largefiles, and No async I/O, 
# pread extensions
# For older SPP-UX machines with 9.05 compatibility
#
sppux:	iozone_sppux.o  libbif.o
	$(NACC)  -O $(LDFLAGS) iozone_sppux.o  libbif.o \
	-Wl,+parallel -lcnx_syscall -lpthread -lail -o iozone

#
# SPP-UX 32 bit build with threads, No largefiles, and No async I/O, pread 
# extensions
# For Newer SPP-UX machines with 10.01 compatibility
#
sppux-10.1:	iozone_sppux-10.1.o libbif.o
	$(NACC) -O $(LDFLAGS) iozone_sppux-10.1.o libbif.o \
	 -lcnx_syscall  -Wl,+parallel -lpthread -lail -o iozone

#
# SPP-UX 32 bit build with threads, No largefiles, and No async I/O, pread 
# extensions
# For Newer SPP-UX machines with 10.01 compatibility, and no ansi 'C' compiler.
#
sppux_no_ansi-10.1:	iozone_sppux_no-10.1.o libbif.o
	$(CCS)  -O $(LDFLAGS) iozone_sppux_no-10.1.o libbif.o \
		-Wl,+parallel -lcnx_syscall  \
		-lpthread -lail -o iozone

#
# Convex 'C' series 32 bit build with No threads, No largefiles, and No async I/O
#
convex:	iozone_convex.o libbif.o
	$(CC) -O $(LDFLAGS)iozone_convex.o libbif.o -o iozone

#
# Solaris 32 bit build with threads, largefiles, and async I/O
#
Solaris:	iozone_solaris.o libasync.o libbif.o fileop_Solaris.o pit_server.o
	$(CC)  -O $(LDFLAGS) iozone_solaris.o libasync.o libbif.o \
		-lthread -lpthread -lposix4 -lnsl -laio -lsocket \
		-o iozone
	$(CC)  -O fileop_Solaris.o -o fileop
	$(CC)  -O pit_server.o -lthread -lpthread -lposix4 -lnsl -laio \
		-lsocket -o pit_server

#
# Solaris 32 bit build with threads, largefiles, and async I/O
#
Solaris7gcc:	iozone_solaris7gcc.o libasync7.o libbif7.o 
	$(GCC)  -O $(LDFLAGS) iozone_solaris7gcc.o libasync7.o libbif7.o \
		-lthread -lpthread -lposix4 -lnsl -laio \
		-lsocket -o iozone
#
# Solaris 32 bit build with threads, largefiles, and async I/O
#
Solaris10:	iozone_solaris10.o libasync10.o libbif10.o fileop_Solaris10.o pit_server.o
	$(CC)  -O $(LDFLAGS) iozone_solaris10.o libasync10.o libbif10.o \
		-lthread -lpthread -lposix4 -lnsl -laio \
		-lsocket -o iozone
	$(CC)  -O fileop_Solaris10.o -o fileop
	$(CC)  -O pit_server.o -lthread -lpthread -lposix4 -lnsl -laio \
                -lsocket -o pit_server

#
# Solaris 32 bit build with threads, largefiles, and async I/O
#
Solaris10cc:	iozone_solaris10cc.o libasync10cc.o libbif10cc.o fileop_Solaris10cc.o pit_server.o
	$(CC)  -O $(LDFLAGS) iozone_solaris10cc.o libasync10cc.o libbif10cc.o \
		-lthread -lpthread -lposix4 -lnsl -laio \
		-lsocket -o iozone
	$(CC)  -O fileop_Solaris10cc.o -o fileop
	$(CC)  -O pit_server.o -lthread -lpthread -lposix4 -lnsl -laio \
                -lsocket -o pit_server

#
# Solaris 32 bit build with threads, largefiles, and async I/O
#
Solaris10gcc:	iozone_solaris10gcc.o libasync10.o libbif10.o fileop_Solaris10gcc.o pit_server_solaris10gcc.o
	$(GCC)  -O $(LDFLAGS) iozone_solaris10gcc.o libasync10.o libbif10.o \
		-lthread -lpthread -lposix4 -lnsl -laio \
		-lsocket -o iozone
	$(GCC)  -O fileop_Solaris10gcc.o -o fileop
	$(GCC)  -O pit_server_solaris10gcc.o -lthread -lpthread -lposix4 -lnsl -laio \
                -lsocket -o pit_server

#
# Solaris 64 bit build with threads, largefiles, and async I/O
#
Solaris10gcc-64:	iozone_solaris10gcc-64.o libasync10-64.o libbif10-64.o fileop_Solaris10gcc-64.o pit_server_solaris10gcc-64.o
	$(GCC)  -O $(LDFLAGS) $(S10GCCFLAGS) iozone_solaris10gcc-64.o libasync10-64.o libbif10-64.o \
		-lthread -lpthread -lposix4 -lnsl -laio \
		-lsocket -o iozone
	$(GCC)  -O $(S10GCCFLAGS) fileop_Solaris10gcc-64.o -o fileop
	$(GCC)  -O $(S10GCCFLAGS) pit_server_solaris10gcc-64.o -lthread -lpthread -lposix4 \
		-lnsl -laio -lsocket -o pit_server


#
# Solaris 64 bit build with threads, largefiles, and async I/O
#
Solaris10cc-64:	iozone_solaris10cc-64.o libasync10-64.o libbif10-64.o fileop_Solaris10cc-64.o pit_server.o
	$(CC)  -O $(LDFLAGS) $(S10CCFLAGS) iozone_solaris10cc-64.o libasync10-64.o libbif10-64.o \
              -lthread -lpthread -lposix4 -lnsl -laio \
              -lsocket -o iozone
	$(CC)  -O $(S10CCFLAGS) fileop_Solaris10cc-64.o -o fileop
	$(CC)  -O $(S10CCFLAGS) pit_server.o -lthread -lpthread -lposix4 \
		-lnsl -laio -lsocket -o pit_server



#
# Solaris 2.6 (32 bit) build with no threads, no largefiles, and no async I/O
#
Solaris-2.6:	iozone_solaris-2.6.o libbif.o 
	$(CC)  -O $(LDFLAGS) iozone_solaris-2.6.o libbif.o \
		-lnsl -laio -lsocket -o iozone

#
# Solaris 64 bit build with threads, largefiles, and async I/O
#
Solaris8-64: iozone_solaris8-64.o libasync.o libbif.o
	$(CC) $(LDFLAGS) -fast -xtarget=generic64 -v iozone_solaris8-64.o \
		libasync.o libbif.o -lthread -lpthread -lposix4 -lnsl -laio \
		-lsocket -o iozone

#
# Solaris 64 bit build with threads, largefiles, async I/O, and Vxfs
#
Solaris8-64-VXFS: iozone_solaris8-64-VXFS.o libasync.o libbif.o
	$(CC) $(LDFLAGS) -fast -xtarget=generic64 -v -I/opt/VRTSvxfs/include/ 
		iozone_solaris8-64-VXFS.o libasync.o libbif.o \
		-lthread -lpthread -lposix4 -lnsl -laio \
		-lsocket -o iozone

#
# Windows build requires Cygwin development environment. You
# can get this from www.cygwin.com
# No largefiles, No async I/O
#
Windows:	iozone_windows.o libbif.o fileop_windows.o pit_server_win.o
	$(GCC) -O $(LDFLAGS) iozone_windows.o libbif.o -o iozone
	$(GCC) -O $(LDFLAGS) fileop_windows.o -o fileop
	$(GCC) -O $(LDFLAGS) pit_server_win.o -o pit_server

#
# Windows build requires SUA development environment. You
# can get this from Microsoft
# No largefiles, No async I/O
#
SUA:	iozone_sua.o libbif.o fileop_sua.o pit_server_sua.o
	$(GCC) -O $(LDFLAGS) iozone_sua.o libbif.o -o iozone
	$(GCC) -O $(LDFLAGS) fileop_sua.o -o fileop
	$(GCC) -O $(LDFLAGS) pit_server_sua.o -o pit_server

#
# Uwin build requires UWIN development environment. 
# No threads, No largefiles, No async I/O
#
UWIN:	iozone_uwin.o libbif.o
	$(GCC) -O $(LDFLAGS) iozone_uwin.o libbif.o -o iozone

#
# GNU C compiler BSD/OS build with threads, largefiles, no async I/O
#

bsdi:	iozone_bsdi.o libbif.o fileop_bsdi.o pit_server.o
	$(CC) -O $(LDFLAGS) iozone_bsdi.o libbif.o -o iozone
	$(CC) -O fileop_bsdi.o -o fileop
	$(CC) -O pit_server.o -o pit_server

#
# GNU C compiler FreeBSD build with no threads, no largefiles, no async I/O
#

freebsd:	iozone_freebsd.o libbif.o fileop_freebsd.o libasync.o pit_server.o
	$(CC) $(LDFLAGS) iozone_freebsd.o libbif.o -lpthread libasync.o \
		-o iozone
	$(CC)  -O fileop_freebsd.o -o fileop
	$(CC)  -O pit_server.o -o pit_server

#
# GNU C compiler DragonFly build with no threads, no largefiles
#
dragonfly:	iozone_dragonfly.o libbif.o fileop_dragonfly.o pit_server.o
	$(CC) $(LDFLAGS) iozone_dragonfly.o libbif.o -o iozone
	$(CC)  -O fileop_dragonfly.o -o fileop
	$(CC)  -O pit_server.o -o pit_server

#
# GNU C compiler MacosX build with no threads, no largefiles, no async I/O
#

macosx:	iozone_macosx.o fileop_macosx.o pit_server.o
	$(CC) -O $(LDFLAGS) iozone_macosx.o libbif.o -o iozone
	$(CC) -O $(LDFLAGS) fileop_macosx.o -o fileop
	$(CC) -O $(LDFLAGS) pit_server.o -o pit_server
#
#
# GNU C compiler OpenBSD build with no threads, no largefiles, no async I/O
#

openbsd:	iozone_openbsd.o libbif.o fileop_openbsd.o pit_server.o
	$(CC) -O $(LDFLAGS) iozone_openbsd.o libbif.o -o iozone
	$(CC)  -O fileop_openbsd.o -o fileop
	$(CC)  -O pit_server.o -o pit_server

#
# GNU C compiler OpenBSD build with threads, no largefiles, no async I/O
#

openbsd-threads:	iozone_openbsd-threads.o libbif.o
	$(CC) -O $(LDFLAGS) -pthread iozone_openbsd-threads.o \
		libbif.o -o iozone

#
# GNU C compiler OSFV3 build 
# Has threads and async I/O but no largefiles.
#

OSFV3:	iozone_OSFV3.o libbif.o libasync.o
	$(CC) -O $(LDFLAGS) iozone_OSFV3.o libbif.o \
		-lpthreads libasync.o -laio -o iozone

#
# GNU C compiler OSFV4 build 
# Has threads and async I/O but no largefiles.
#

OSFV4:	iozone_OSFV4.o libbif.o libasync.o
	$(CC) -O $(LDFLAGS) iozone_OSFV4.o libbif.o -lpthread \
		libasync.o -laio -o iozone

#
# GNU C compiler OSFV5 build 
# Has threads and async I/O but no largefiles.
#

OSFV5:	iozone_OSFV5.o libbif.o libasync.o
	$(CC) -O $(LDFLAGS) iozone_OSFV5.o libbif.o -lpthread \
		libasync.o -laio -o iozone

#
# GNU C compiler TRU64 build 
# Has threads and async I/O but no largefiles.
#

TRU64:	iozone_TRU64.o libbif.o libasync.o
	$(CC) -O $(LDFLAGS) iozone_TRU64.o libbif.o -lpthread \
		libasync.o -laio -o iozone

#
# GNU Generic build with no threads, no largefiles, no async I/O
# for SCO
# Note:	Be sure you have the latest patches for SCO's Openserver
# or you will get warnings about timer problems.
#

SCO:	iozone_SCO.o  libbif.o
	$(GCC) -O $(LDFLAGS) iozone_SCO.o -lsocket -s libbif.o -o iozone


#
# GNU build with threads, largefiles, async I/O
# for SCO Unixware 5 7.1.1 i386 x86at SCO UNIX SVR5
# Note: Be sure you have the latest patches for SCO's Openserver
# or you will get warnings about timer problems.
#

SCO_Unixware_gcc:	iozone_SCO_Unixware_gcc.o  libbif.o libasync.o
	$(GCC) -O $(LDFLAGS) iozone_SCO_Unixware_gcc.o libbif.o libasync.o \
		-lsocket -lthread -o iozone

#
# GNU C compiler NetBSD build with no threads, no largefiles, no async I/O
#

netbsd:	iozone_netbsd.o  libbif.o fileop_netbsd.o pit_server.o
	$(CC) -O $(LDFLAGS) iozone_netbsd.o libbif.o -o iozone
	$(CC) -O fileop_netbsd.o -o fileop
	$(CC) -O pit_server.o -o pit_server

#
#
# Now for the machine specific stuff
#

iozone_hpux.o:	iozone.c libbif.c
	@echo ""
	@echo "Building iozone for HP-UX (9.05)"
	@echo ""
	$(C89) +e -c -O -Dunix -D_HPUX_SOURCE -DHAVE_ANSIC_C -DNO_THREADS \
		-DNAME='"hpux"' $(CFLAGS) iozone.c  -o iozone_hpux.o
	$(C89) +e -c -O -Dunix -D_HPUX_SOURCE -DHAVE_ANSIC_C -DNO_THREADS \
		$(CFLAGS) libbif.c -o libbif.o

iozone_hpux-11.0.o:	iozone.c libasync.c libbif.c
	@echo ""
	@echo "Building iozone for HP-UX (11.0)"
	@echo ""
	$(CC) -c +O3 +Oparallel -Dunix -D_LARGEFILE64_SOURCE  -D_HPUX_SOURCE \
		-DNAME='"hpux-11.0"' -DHAVE_ANSIC_C -DASYNC_IO -DVXFS $(CFLAGS) \
		iozone.c  -o iozone_hpux-11.0.o
	$(CC) -c  +O3 +Oparallel -Dunix -D_LARGEFILE64_SOURCE  -D_HPUX_SOURCE \
		-DHAVE_ANSIC_C -DASYNC_IO -DVXFS $(CFLAGS) libasync.c  -o libasync.o
	$(CC) -c  +O3 +Oparallel -Dunix -D_LARGEFILE64_SOURCE  -D_HPUX_SOURCE \
		-DHAVE_ANSIC_C -DASYNC_IO -DVXFS $(CFLAGS) libbif.c  -o libbif.o

iozone_hpux-11.0w.o:	iozone.c libasync.c libbif.c
	@echo ""
	@echo "Building iozone for HP-UX (11.0w)"
	@echo ""
	$(CC) -c +DD64 +O3 -Dunix -D_LARGEFILE64_SOURCE  -D_HPUX_SOURCE \
	    -DNAME='"hpux-11.0w"' -DHAVE_ANSIC_C -DASYNC_IO -DVXFS $(CFLAGS) iozone.c \
	     -o iozone_hpux-11.0w.o
	$(CC) -c +DD64 +O3 -Dunix -D_LARGEFILE64_SOURCE  -D_HPUX_SOURCE \
		-DHAVE_ANSIC_C -DASYNC_IO -DVXFS $(CFLAGS) libasync.c  -o libasyncw.o
	$(CC) -c +DD64 +O3 -Dunix -D_LARGEFILE64_SOURCE  -D_HPUX_SOURCE \
		-DHAVE_ANSIC_C -DASYNC_IO -DVXFS $(CFLAGS) libbif.c  -o libbif.o

iozone_hpuxs-11.0.o:	iozone.c libasync.c libbif.c
	@echo ""
	@echo "Building simple iozone for HP-UX (11.0)"
	@echo ""
	$(CC) -c  -Dunix -D_LARGEFILE64_SOURCE  -D_HPUX_SOURCE -DHAVE_ANSIC_C \
		-DNAME='"hpuxs-11.0"' -DASYNC_IO -DVXFS -DHAVE_PREAD $(CFLAGS) iozone.c  \
		-o iozone_hpuxs-11.0.o
	$(CC) -c  -Dunix -D_LARGEFILE64_SOURCE  -D_HPUX_SOURCE -DHAVE_ANSIC_C \
		-DASYNC_IO  -DVXFS $(CFLAGS) libasync.c  -o libasync.o 
	$(CC) -c  -Dunix -D_LARGEFILE64_SOURCE  -D_HPUX_SOURCE -DHAVE_ANSIC_C \
		-DASYNC_IO -DVXFS $(CFLAGS) libbif.c  -o libbif.o 

fileop_hpuxs-11.0.o:	fileop.c
	@echo ""
	@echo "Building simple fileop for HP-UX (11.0)"
	@echo ""
	$(CC) -c  $(CFLAGS) fileop.c  -o fileop_hpuxs-11.0.o 

pit_server_solaris10gcc-64.o:	pit_server.c
	@echo ""
	@echo "Building the pit_server"
	@echo ""
	$(GCC) -c  $(CFLAGS) $(S10GCCFLAGS) pit_server.c  -o pit_server_solaris10gcc-64.o 

pit_server.o:	pit_server.c
	@echo ""
	@echo "Building the pit_server"
	@echo ""
	$(CC) -c  $(CFLAGS) pit_server.c  -o pit_server.o 

pit_server-linux-powerpc64.o:	pit_server.c
	@echo ""
	@echo "Building the pit_server"
	@echo ""
	$(CC) -c  $(CFLAGS) $(FLAG64BIT) pit_server.c -o pit_server-linux-powerpc64.o 

pit_server_solaris10gcc.o:	pit_server.c
	@echo ""
	@echo "Building the pit_server"
	@echo ""
	$(GCC) -c  $(CFLAGS) pit_server.c -o pit_server_solaris10gcc.o 


pit_server_win.o:	pit_server.c
	@echo ""
	@echo "Building the pit_server for Windows"
	@echo ""
	$(GCC) -c  $(CFLAGS) -DWindows pit_server.c  -o pit_server_win.o 

pit_server_sua.o:	pit_server.c
	@echo ""
	@echo "Building the pit_server for Windows SUA"
	@echo ""
	$(GCC) -c  $(CFLAGS) -D_SUA_ pit_server.c  -o pit_server_sua.o 

iozone_hpuxs-11.0w.o:	iozone.c libasync.c libbif.c
	@echo ""
	@echo "Building simple iozone for HP-UX (11.0w)"
	@echo ""
	$(CC) -c +DD64 -Dunix -D_LARGEFILE64_SOURCE  -D_HPUX_SOURCE \
		-DNAME='"hpuxs-11.0w"' -DHAVE_ANSIC_C -DASYNC_IO -DVXFS \
		-DHAVE_PREAD $(CFLAGS) iozone.c  -o iozone_hpuxs-11.0w.o
	$(CC) -c +DD64 -Dunix -D_LARGEFILE64_SOURCE  -D_HPUX_SOURCE \
		-DHAVE_ANSIC_C -DASYNC_IO -DVXFS $(CFLAGS) libasync.c  -o libasyncw.o 
	$(CC) -c +DD64 -Dunix -D_LARGEFILE64_SOURCE  -D_HPUX_SOURCE \
		-DHAVE_ANSIC_C -DASYNC_IO -DVXFS $(CFLAGS) libbif.c  -o libbif.o 

iozone_hpux-10.1.o:	iozone.c libbif.c
	@echo ""
	@echo "Building iozone for HP-UX (10.1)"
	@echo ""
	$(C89) +e -c -O -Dunix -D_HPUX_SOURCE -DHAVE_ANSIC_C -DNO_THREADS \
		-DNAME='"hpux-10.1"' $(CFLAGS) iozone.c  -o iozone_hpux-10.1.o
	$(C89) +e -c -O -Dunix -D_HPUX_SOURCE -DHAVE_ANSIC_C -DNO_THREADS \
		$(CFLAGS) libbif.c  -o libbif.o

iozone_hpux-10.20.o:	iozone.c libbif.c
	@echo ""
	@echo "Building iozone for HP-UX (10.20)"
	@echo ""
	$(C89) +e -c -O -Dunix -D_HPUX_SOURCE -DHAVE_ANSIC_C -DNO_THREADS \
		-DNAME='"hpux-10.20"' $(CFLAGS) iozone.c  -o iozone_hpux-10.20.o
	$(C89) +e -c -O -Dunix -D_HPUX_SOURCE -DHAVE_ANSIC_C -DNO_THREADS \
		$(CFLAGS) libbif.c  -o libbif.o

iozone_ghpux.o:	iozone.c libbif.c
	@echo ""
	@echo "Building iozone for GCC HP-UX (9.05) "
	@echo ""
	$(GCC) -c -O -Dunix -D_HPUX_SOURCE -DHAVE_ANSIC_C -DNO_THREADS $(CFLAGS) iozone.c \
		-DNAME='"h=ghpux"' -o iozone_ghpux.o
	$(GCC) -c -O -Dunix -D_HPUX_SOURCE -DHAVE_ANSIC_C -DNO_THREADS \
		$(CFLAGS) libbif.c -o libbif.o

iozone_generic.o:	iozone.c libbif.c
	@echo ""
	@echo "Building iozone Generic "
	@echo ""
	$(CC) -c -O -Dgeneric -Dunix -DHAVE_ANSIC_C -DNO_THREADS \
		-DNAME='"Generic"' $(CFLAGS) iozone.c -o iozone_generic.o
	$(CC) -c -O -Dgeneric -Dunix -DHAVE_ANSIC_C -DNO_THREADS \
		$(CFLAGS) libbif.c -o libbif.o

iozone_hpux_no.o:	iozone.c libbif.c
	@echo ""
	@echo "Building iozone for HP-UX (9.05) without ansi compiler"
	@echo ""
	$(NACC) -c -O -Dunix -D_HPUX_SOURCE -DNO_THREADS iozone.c \
		-DNAME='"hpux_no_ansi"' -o iozone_hpux_no.o
	$(NACC) -c -O -Dunix -D_HPUX_SOURCE -DNO_THREADS \
		libbif.c -o libbif.o

iozone_hpux_no-10.1.o:	iozone.c
	@echo ""
	@echo "Building iozone for HP-UX (10.1) without ansi compiler"
	@echo ""
	$(NACC) -c -O -Dunix -D_HPUX_SOURCE -DNO_THREADS iozone.c \
		-DNAME='"hpux_no_ansi_10.1"' -o iozone_hpux_no-10.1.o
	$(NACC) -c -O -Dunix -D_HPUX_SOURCE -DNO_THREADS \
		libbif.c -o libbif.o

iozone_linux-powerpc.o:	iozone.c libbif.c libasync.c
	@echo ""
	@echo "Building iozone for Linux PowerPC"
	@echo ""
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DDONT_HAVE_O_DIRECT \
		-DSHARED_MEM -Dlinux -D_LARGEFILE64_SOURCE $(CFLAGS) iozone.c \
		-DNAME='"linux-powerpc"' -o iozone_linux-powerpc.o
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D_LARGEFILE64_SOURCE \
		-DSHARED_MEM -Dlinux $(CFLAGS) libbif.c -o libbif.o
	$(CC) -c -O3 -Dunix -Dlinux -DHAVE_ANSIC_C -DASYNC_IO \
		-D_LARGEFILE64_SOURCE $(CFLAGS) libasync.c  -o libasync.o 

iozone_linux-powerpc64.o:	iozone.c libbif.c libasync.c
	@echo ""
	@echo "Building iozone for Linux PowerPC64"
	@echo ""
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DNAME='"linux-powerpc64"' \
		-DSHARED_MEM -Dlinux -D_LARGEFILE64_SOURCE $(CFLAGS) iozone.c \
		$(FLAG64BIT) -o iozone_linux-powerpc64.o
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D_LARGEFILE64_SOURCE \
		-DSHARED_MEM -Dlinux $(CFLAGS) libbif.c $(FLAG64BIT) -o libbif.o
	$(CC) -c -O3 -Dunix -Dlinux -DHAVE_ANSIC_C -DASYNC_IO \
		-D_LARGEFILE64_SOURCE $(CFLAGS) libasync.c $(FLAG64BIT) -o libasync.o 
		

iozone_linux-sparc.o:	iozone.c libbif.c libasync.c
	@echo ""
	@echo "Building iozone for Linux Sparc"
	@echo ""
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DDONT_HAVE_O_DIRECT \
		-DSHARED_MEM -Dlinux -D_LARGEFILE64_SOURCE $(CFLAGS) iozone.c \
		-DNAME='"linux-sparc"' -o iozone_linux-sparc.o
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D_LARGEFILE64_SOURCE \
		-DSHARED_MEM -Dlinux $(CFLAGS) libbif.c -o libbif.o
	$(CC) -c -O3 -Dunix -Dlinux -DHAVE_ANSIC_C -DASYNC_IO \
		-D_LARGEFILE64_SOURCE $(CFLAGS) libasync.c  -o libasync.o 

iozone_linux.o:	iozone.c libbif.c libasync.c
	@echo ""
	@echo "Building iozone for Linux"
	@echo ""
	$(CC) -Wall -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DHAVE_PREAD \
		-DSHARED_MEM -Dlinux -D_LARGEFILE64_SOURCE $(CFLAGS) iozone.c \
		-DNAME='"linux"' -o iozone_linux.o
	$(CC) -Wall -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D_LARGEFILE64_SOURCE \
		-DSHARED_MEM -Dlinux $(CFLAGS) libbif.c -o libbif.o
	$(CC) -Wall -c -O3 -Dunix -Dlinux -DHAVE_ANSIC_C -DASYNC_IO \
		-D_LARGEFILE64_SOURCE $(CFLAGS) libasync.c  -o libasync.o 

fileop_AIX.o:	fileop.c
	@echo ""
	@echo "Building fileop for AIX"
	@echo ""
	$(CC) -c -O $(CFLAGS) fileop.c -o fileop_AIX.o

fileop_AIX-LF.o:	fileop.c
	@echo ""
	@echo "Building fileop for AIX-LF"
	@echo ""
	$(CC) -c -O $(CFLAGS) fileop.c -o fileop_AIX-LF.o

fileop_AIX64.o:       fileop.c
	@echo ""
	@echo "Building fileop for AIX64"
	@echo ""
	$(GCC) -maix64 -c -O3 $(CFLAGS) fileop.c -o fileop_AIX64.o

fileop_AIX64-LF.o:    fileop.c
	@echo ""
	@echo "Building fileop for AIX64-LF"
	@echo ""
	$(GCC) -maix64 -c -O3 $(CFLAGS) fileop.c -o fileop_AIX64-LF.o

fileop_bsdi.o:	fileop.c
	@echo ""
	@echo "Building fileop for BSDi"
	@echo ""
	$(CC) -c -O $(CFLAGS) fileop.c -o fileop_bsdi.o

fileop_freebsd.o:	fileop.c
	@echo ""
	@echo "Building fileop for FreeBSD"
	@echo ""
	$(CC) -c -O $(CFLAGS) fileop.c -o fileop_freebsd.o

fileop_dragonfly.o:	fileop.c
	@echo ""
	@echo "Building fileop for DragonFly"
	@echo ""
	$(CC) -c -O $(CFLAGS) fileop.c -o fileop_dragonfly.o

fileop_netbsd.o:	fileop.c
	@echo ""
	@echo "Building fileop for NetBSD"
	@echo ""
	$(CC) -c -O $(CFLAGS) fileop.c -o fileop_netbsd.o

fileop_Solaris.o:	fileop.c
	@echo ""
	@echo "Building fileop for Solaris"
	@echo ""
	$(CC) -c -O $(CFLAGS) fileop.c -o fileop_Solaris.o

fileop_Solaris10.o:	fileop.c
	@echo ""
	@echo "Building fileop for Solaris10"
	@echo ""
	$(CC) -c -O $(CFLAGS) fileop.c -o fileop_Solaris10.o

fileop_Solaris10cc.o: fileop.c
	@echo ""
	@echo "Building fileop for Solaris10cc"
	@echo ""
	$(CC) -c -O $(CFLAGS) fileop.c -o fileop_Solaris10cc.o


fileop_Solaris10gcc.o:	fileop.c
	@echo ""
	@echo "Building fileop for Solaris10gcc"
	@echo ""
	$(GCC) -c -O $(CFLAGS) fileop.c -o fileop_Solaris10gcc.o

fileop_Solaris10gcc-64.o:	fileop.c
	@echo ""
	@echo "Building fileop for Solaris10gcc-64"
	@echo ""
	$(GCC) -c -O $(CFLAGS) $(S10GCCFLAGS) fileop.c -o fileop_Solaris10gcc-64.o

fileop_Solaris10cc-64.o:      fileop.c
	@echo ""
	@echo "Building fileop for Solaris10cc-64"
	@echo ""
	$(CC) -c -O $(CFLAGS) $(S10CCFLAGS) fileop.c -o fileop_Solaris10cc-64.o


fileop_linux.o:	fileop.c
	@echo ""
	@echo "Building fileop for Linux"
	@echo ""
	$(CC) -Wall -c -O3 $(CFLAGS) fileop.c -o fileop_linux.o

fileop_openbsd.o:	fileop.c
	@echo ""
	@echo "Building fileop for OpenBSD"
	@echo ""
	$(CC) -Wall -c -O $(CFLAGS) fileop.c -o fileop_openbsd.o

fileop_macosx.o:	fileop.c
	@echo ""
	@echo "Building fileop for MAC OS X"
	@echo ""
	$(CC) -Wall -c -O -DIOZ_macosx $(CFLAGS) fileop.c -o fileop_macosx.o

fileop_linux-ia64.o:	fileop.c
	@echo ""
	@echo "Building fileop for Linux-ia64"
	@echo ""
	$(CC) -Wall -c -O3 $(CFLAGS) fileop.c -o fileop_linux-ia64.o

fileop_linux-ppc.o:	fileop.c
	@echo ""
	@echo "Building fileop for Linux-powerpc"
	@echo ""
	$(CC) -Wall -c -O3 $(CFLAGS) fileop.c -o fileop_linux-ppc.o

fileop_linux-ppc64.o:	fileop.c
	@echo ""
	@echo "Building fileop for Linux-powerpc64"
	@echo ""
	$(CC) -Wall -c -O3 $(CFLAGS) $(FLAG64BIT) fileop.c -o fileop_linux-ppc64.o

fileop_linux-AMD64.o:	fileop.c
	@echo ""
	@echo "Building fileop for Linux-AMD64"
	@echo ""
	$(CC) -Wall -c -O3 $(CFLAGS) fileop.c -o fileop_linux-AMD64.o

fileop_linux-arm.o:  fileop.c
	@echo ""
	@echo "Building fileop for Linux-arm"
	@echo ""
	$(GCC) -Wall -c -O3 $(CFLAGS) fileop.c -o fileop_linux-arm.o

fileop_linux-s390.o:  fileop.c
	@echo ""
	@echo "Building fileop for Linux-S390"
	@echo ""
	$(GCC) -Wall -c -O3 $(CFLAGS) fileop.c -o fileop_linux-s390.o

fileop_linux-s390x.o:  fileop.c
	@echo ""
	@echo "Building fileop for Linux-s390x"
	@echo ""
	$(GCC) -Wall -c -O3 $(CFLAGS) fileop.c -o fileop_linux-s390x.o

fileop_windows.o: fileop.c
	@echo ""
	@echo "Building fileop for Windows"
	@echo ""
	$(GCC) -Wall -c -O3 $(CFLAGS) -DWindows fileop.c -o fileop_windows.o

fileop_sua.o: fileop.c
	@echo ""
	@echo "Building fileop for Windows SUA"
	@echo ""
	$(GCC) -Wall -c -O3 $(CFLAGS) -D_SUA_ fileop.c -o fileop_sua.o

iozone_linux-ia64.o:	iozone.c libbif.c libasync.c
	@echo ""
	@echo "Building iozone for Linux-ia64"
	@echo ""
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DNAME='"linux-ia64"' \
		-DSHARED_MEM -Dlinux -D_LARGEFILE64_SOURCE $(CFLAGS) iozone.c \
		-o iozone_linux-ia64.o
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D_LARGEFILE64_SOURCE \
		-DSHARED_MEM -Dlinux $(CFLAGS) libbif.c -o libbif.o
	$(CC) -c -O3 -Dunix -Dlinux -DHAVE_ANSIC_C -DASYNC_IO \
		-D_LARGEFILE64_SOURCE $(CFLAGS) libasync.c  -o libasync.o 

iozone_linux-arm.o:	iozone.c libbif.c libasync.c
	@echo ""
	@echo "Building iozone for Linux-arm"
	@echo ""
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DHAVE_PREAD \
		-DNAME='"linux-arm"' -DLINUX_ARM -DSHARED_MEM \
		-Dlinux -D_LARGEFILE64_SOURCE $(CFLAGS) iozone.c \
		-o iozone_linux-arm.o
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D_LARGEFILE64_SOURCE \
		-DSHARED_MEM -Dlinux $(CFLAGS) libbif.c -o libbif.o
	$(CC) -c -O3 -Dunix -Dlinux -DHAVE_ANSIC_C -DASYNC_IO \
		-D_LARGEFILE64_SOURCE $(CFLAGS) libasync.c  -o libasync.o

iozone_linux-AMD64.o:	iozone.c libbif.c libasync.c 
	@echo ""
	@echo "Building iozone for Linux-AMD64"
	@echo ""
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DNAME='"linux-AMD64"' \
		-D__AMD64__ -DSHARED_MEM -Dlinux -D_LARGEFILE64_SOURCE \
		-DHAVE_PREAD $(CFLAGS) iozone.c -o iozone_linux-AMD64.o
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D_LARGEFILE64_SOURCE \
		-DSHARED_MEM -Dlinux $(CFLAGS) libbif.c -o libbif.o
	$(CC) -c -O3 -Dunix -Dlinux -DHAVE_ANSIC_C -DASYNC_IO \
		-D_LARGEFILE64_SOURCE $(CFLAGS) libasync.c  -o libasync.o 

iozone_linux-s390.o:	iozone.c libbif.c libasync.c
	@echo ""
	@echo "Building iozone for Linux-s390"
	@echo ""
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DHAVE_PREAD \
		-DSHARED_MEM -Dlinux -D_LARGEFILE64_SOURCE $(CFLAGS) iozone.c \
		-DNAME='"linux-s390"' -o iozone_linux-s390.o
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DSHARED_MEM -Dlinux \
		-D_LARGEFILE64_SOURCE $(CFLAGS) libbif.c -o libbif.o
	$(CC) -c -O3 -Dunix -Dlinux -DHAVE_ANSIC_C -DASYNC_IO \
		-D_LARGEFILE64_SOURCE $(CFLAGS) libasync.c -o libasync.o

iozone_linux-s390x.o:	iozone.c libbif.c libasync.c
	@echo ""
	@echo "Building iozone for Linux-s390x"
	@echo ""
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DHAVE_PREAD \
		-DSHARED_MEM -Dlinux -D_LARGEFILE64_SOURCE $(CFLAGS) iozone.c \
		-DNAME='"linux-s390x"' -o iozone_linux-s390x.o
	$(CC) -c -O3 -Dunix -DHAVE_ANSIC_C -DSHARED_MEM -Dlinux \
		-D_LARGEFILE64_SOURCE $(CFLAGS) libbif.c -o libbif.o
	$(CC) -c -O3 -Dunix -Dlinux -DHAVE_ANSIC_C -DASYNC_IO \
		-D_LARGEFILE64_SOURCE $(CFLAGS) libasync.c -o libasync.o


iozone_AIX.o:	iozone.c libbif.c 
	@echo ""
	@echo "Building iozone for AIX"
	@echo ""
	$(CC) -c -O -D__AIX__ -D_NO_PROTO -Dunix -DHAVE_ANSIC_C  \
		-DNAME='"AIX"' -DSHARED_MEM  $(CFLAGS) iozone.c -o iozone_AIX.o
	$(CC) -c -O -D__AIX__ -D_NO_PROTO -Dunix -DHAVE_ANSIC_C  \
		-DSHARED_MEM $(CFLAGS) libbif.c -o libbif.o

iozone_AIX-LF.o:	iozone.c libbif.c 
	@echo ""
	@echo "Building iozone for AIX with Large files"
	@echo ""
	$(CC) -c -O -D__AIX__ -D_NO_PROTO -Dunix -DHAVE_ANSIC_C  \
		-DSHARED_MEM  -D_LARGEFILE64_SOURCE -D_LARGE_FILES \
		-DNAME='"AIX-LF"' $(CFLAGS) iozone.c -o iozone_AIX-LF.o
	$(CC) -c -O -D__AIX__ -D_NO_PROTO -Dunix -DHAVE_ANSIC_C  \
		-DSHARED_MEM -D_LARGEFILE64_SOURCE -D_LARGE_FILES \
		$(CFLAGS) libbif.c -o libbif.o

iozone_AIX64.o:       iozone.c libbif.c libasync.c
	@echo ""
	@echo "Building iozone for AIX64"
	@echo ""
	$(GCC) -maix64 -c -O3 -D__AIX__ -D_NO_PROTO -Dunix -DHAVE_ANSIC_C \
              -DASYNC_IO -DNAME='"AIX64"' -DSHARED_MEM \
              $(CFLAGS) iozone.c -o iozone_AIX64.o
	$(GCC) -maix64 -c -O3 -D__AIX__ -D_NO_PROTO -Dunix -DHAVE_ANSIC_C \
              -DASYNC_IO -DSHARED_MEM $(CFLAGS) libbif.c -o libbif.o
	$(GCC) -maix64 -c -O3 -Dunix -Dlinux -DHAVE_ANSIC_C -DASYNC_IO \
              $(CFLAGS) libasync.c -o libasync.o

iozone_AIX64-LF.o:    iozone.c libbif.c libasync.c
	@echo ""
	@echo "Building iozone for AIX64 with Large files"
	@echo ""
	$(GCC) -maix64 -c -O3 -D__AIX__ -D_NO_PROTO -Dunix -DHAVE_ANSIC_C \
              -DASYNC_IO -DNAME='"AIX64-LF"' -DSHARED_MEM \
              -D_LARGEFILE64_SOURCE -D_LARGE_FILES \
              $(CFLAGS) iozone.c -o iozone_AIX64-LF.o
	$(GCC) -maix64 -c -O3 -D__AIX__ -D_NO_PROTO -Dunix -DHAVE_ANSIC_C \
              -DASYNC_IO -DSHARED_MEM -D_LARGEFILE64_SOURCE -D_LARGE_FILES \
              $(CFLAGS) libbif.c -o libbif.o
	$(GCC) -maix64 -c -O3 -Dunix -Dlinux -DHAVE_ANSIC_C -DASYNC_IO \
              -D_LARGEFILE64_SOURCE -D_LARGE_FILES \
              $(CFLAGS) libasync.c -o libasync.o

iozone_solaris.o:	iozone.c libasync.c libbif.c
	@echo ""
	@echo "Building iozone for Solaris"
	@echo ""
	$(CC) -c -O -Dunix -DHAVE_ANSIC_C -DASYNC_IO \
		-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
		-DNAME='"Solaris"' $(CFLAGS) iozone.c -o iozone_solaris.o
	$(CC) -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D__LP64__ \
		-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
		$(CFLAGS) libasync.c -o libasync.o
	$(CC) -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D__LP64__ \
		-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
		$(CFLAGS) libbif.c -o libbif.o

iozone_solaris7gcc.o:	iozone.c libasync.c libbif.c
	@echo ""
	@echo "Building iozone for Solaris7gcc"
	@echo ""
	$(GCC) -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D__LP64__ \
		-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
		-DNAME='"Solaris7gcc"' $(CFLAGS) libasync.c -o libasync7.o
	$(GCC) -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D__LP64__ \
		-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
		$(CFLAGS) libbif.c -o libbif7.o
	$(GCC) -c -O -Dunix -DHAVE_ANSIC_C -DASYNC_IO \
		-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
		-DNAME='"Solaris7gcc"' $(CFLAGS) iozone.c -o iozone_solaris7gcc.o

iozone_solaris10.o:  iozone.c libbif.c
	@echo ""
	@echo "Building iozone for Solaris10"
	@echo ""
	$(CC) -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D__LP64__ \
	        -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
	        $(CFLAGS) libbif.c -o libbif10.o
	$(CC) -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D__LP64__ \
	        -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
	        -DNAME='"Solaris10"' $(CFLAGS) libasync.c -o libasync10.o
	$(CC) -c -O -Dunix -DHAVE_ANSIC_C -DASYNC_IO -Dstudio11 \
	        -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
	        -DNAME='"Solaris10"' $(CFLAGS) iozone.c -o iozone_solaris10.o

iozone_solaris10cc.o:  iozone.c libbif.c
	@echo ""
	@echo "Building iozone for Solaris10cc"
	@echo ""
	$(CC) -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D__LP64__ \
	        -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
	        $(CFLAGS) libbif.c -o libbif10cc.o
	$(CC) -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D__LP64__ \
	        -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
	        -DNAME='"Solaris10"' $(CFLAGS) libasync.c -o libasync10cc.o
	$(CC) -c -O -Dunix -DHAVE_ANSIC_C -DASYNC_IO -Dstudio11 \
	        -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
	        -DNAME='"Solaris10"' $(CFLAGS) iozone.c -o iozone_solaris10cc.o

iozone_solaris10gcc.o:  iozone.c libbif.c
	@echo ""
	@echo "Building iozone for Solaris10gcc"
	@echo ""
	$(GCC) -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D__LP64__ \
                -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
                $(CFLAGS) libbif.c -o libbif10.o
	$(GCC) -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D__LP64__ \
                -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
                -DNAME='"Solaris10gcc"' $(CFLAGS) libasync.c -o libasync10.o
	$(GCC) -c -O -Dunix -DHAVE_ANSIC_C -DASYNC_IO \
                -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
                -DNAME='"Solaris10gcc"' $(CFLAGS) iozone.c -o iozone_solaris10gcc.o

iozone_solaris10gcc-64.o:  iozone.c libbif.c
	@echo ""
	@echo "Building iozone for Solaris10gcc-64"
	@echo ""
	$(GCC) -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D__LP64__ \
                -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
                $(CFLAGS) $(S10GCCFLAGS) libbif.c -o libbif10-64.o
	$(GCC) -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D__LP64__ \
                -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
                -DNAME='"Solaris10gcc-64"' $(CFLAGS) $(S10GCCFLAGS) libasync.c -o libasync10-64.o
	$(GCC) -c -O -Dunix -DHAVE_ANSIC_C -DASYNC_IO \
                -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
                -DNAME='"Solaris10gcc-64"' $(CFLAGS) $(S10GCCFLAGS) iozone.c -o iozone_solaris10gcc-64.o

iozone_solaris10cc-64.o:  iozone.c libbif.c
	@echo ""
	@echo "Building iozone for Solaris10cc-64"
	@echo ""
	$(CC) -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D__LP64__ \
		-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
		$(CFLAGS) $(S10CCFLAGS) libbif.c -o libbif10-64.o
	$(CC) -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D__LP64__ \
		-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
		-DNAME='"Solaris10cc-64"' $(CFLAGS) $(S10CCFLAGS) libasync.c -o libasync10-64.o
	$(CC) -c -O -Dunix -DHAVE_ANSIC_C -DASYNC_IO -Dstudio11 \
		-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Dsolaris \
		-DNAME='"Solaris10cc-64"' $(CFLAGS) $(S10CCFLAGS) iozone.c -o iozone_solaris10cc-64.o


#
#		-DSHARED_MEM -Dsolaris libasync.c -o libasync.o
#		-DSHARED_MEM -Dsolaris iozone.c -o iozone_solaris.o
#

iozone_solaris-2.6.o:	iozone.c libbif.c
	@echo ""
	@echo "Building iozone for Solaris-2.6"
	@echo ""
	$(CC) -c -O -Dunix -DHAVE_ANSIC_C -DNO_THREADS \
		-DNAME='"Solaris-2.6"' -Dsolaris  $(CFLAGS) iozone.c -o iozone_solaris-2.6.o
	$(CC) -O -c  -Dunix -DHAVE_ANSIC_C \
		-Dsolaris $(CFLAGS) libbif.c -o libbif.o

iozone_solaris8-64.o: iozone.c libasync.c libbif.c
	@echo ""
	@echo "Building iozone for Solaris8-64"
	@echo ""
	$(CC) -fast -xtarget=generic64 -v -c -Dunix -DHAVE_ANSIC_C -DASYNC_IO \
		-D__LP64__ -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
		-DNAME='"Solaris8-64"' -Dsolaris -DHAVE_PREAD \
		$(CFLAGS) iozone.c -o iozone_solaris8-64.o
	$(CC) -fast -xtarget=generic64 -v -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO \
		-D__LP64__ -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
		-Dsolaris -DHAVE_PREAD $(CFLAGS) libasync.c -o libasync.o
	$(CC) -fast -xtarget=generic64 -v -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO \
		-D__LP64__ -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
		-Dsolaris -DHAVE_PREAD $(CFLAGS) libbif.c -o libbif.o

iozone_solaris8-64-VXFS.o: iozone.c libasync.c libbif.c
	@echo ""
	@echo "Building iozone for Solaris8-64-VXFS"
	@echo ""
	$(CC) -fast -xtarget=generic64 -v -c -I/opt/VRTSvxfs/include/ -Dunix \
		-DVXFS -DHAVE_ANSIC_C -DASYNC_IO \
		-D__LP64__ -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
		-DNAME='"Solaris8-64"' -Dsolaris -DHAVE_PREAD \
		$(CFLAGS) iozone.c -o iozone_solaris8-64-VXFS.o
	$(CC) -fast -xtarget=generic64 -v -c  -I/opt/VRTSvxfs/include/ -Dunix \
		-DVXFS -DHAVE_ANSIC_C -DASYNC_IO \
		-D__LP64__ -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
		-Dsolaris -DHAVE_PREAD $(CFLAGS) libasync.c -o libasync.o
	$(CC) -fast -xtarget=generic64 -v -c -I/opt/VRTSvxfs/include/ -Dunix \
		-DVXFS -DHAVE_ANSIC_C -DASYNC_IO \
		-D__LP64__ -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
		-Dsolaris -DHAVE_PREAD $(CFLAGS) libbif.c -o libbif.o

iozone_windows.o:	iozone.c libasync.c libbif.c fileop.c
	@echo ""
	@echo "Building iozone for Windows (No async I/O)"
	@echo ""
	$(GCC) -c -O -Dunix -DHAVE_ANSIC_C -DNO_MADVISE  \
		-DWindows $(CFLAGS) -DDONT_HAVE_O_DIRECT iozone.c \
		-o iozone_windows.o
	$(GCC) -c -O -Dunix -DHAVE_ANSIC_C -DNO_MADVISE \
		-DWindows $(CFLAGS) libbif.c -o libbif.o


#		-D_SUA_ $(CFLAGS) -DDONT_HAVE_O_DIRECT iozone.c \

iozone_sua.o:	iozone.c libasync.c libbif.c fileop.c
	@echo ""
	@echo "Building iozone for Windows SUA (No async I/O)"
	@echo ""
	$(GCC) -c -O -Dunix -DHAVE_ANSIC_C -D_XOPEN_SOURCE -DNO_MADVISE  \
		-D_SUA_ $(CFLAGS) iozone.c \
		-DNAME='"Windows SUA"' -o iozone_sua.o
	$(GCC) -c -O -Dunix -D_SUA_ -D_XOPEN_SOURCE -DHAVE_ANSIC_C \
		-DNO_MADVISE $(CFLAGS) libbif.c -o libbif.o

iozone_uwin.o:	iozone.c libbif.c
	@echo ""
	@echo "Building iozone for UWIN (No threads, No async I/O)"
	@echo ""
	$(GCC) -c -O -DUWIN -Dunix -DHAVE_ANSIC_C -DNO_THREADS  -DNO_MADVISE \
		-DNAME='"UWIN"' -DSHARED_MEM -DWindows $(CFLAGS) iozone.c -o iozone_uwin.o
	$(GCC) -c -O -DUWIN -Dunix -DHAVE_ANSIC_C -DNO_THREADS  -DNO_MADVISE \
		-DSHARED_MEM -DWindows $(CFLAGS) libbif.c -o libbif.o

iozone_IRIX64.o:	iozone.c libasync.c libbif.c
	@echo ""
	@echo "Building iozone for IRIX64"
	@echo ""
	$(CC) -32 -O -c  -Dunix -DHAVE_ANSIC_C -D_LARGEFILE64_SOURCE -DASYNC_IO \
		-DNAME='"IRIX64"' -DIRIX64 -DSHARED_MEM $(CFLAGS) iozone.c -o iozone_IRIX64.o
	$(CC) -32 -O -c  -Dunix -DHAVE_ANSIC_C -D_LARGEFILE64_SOURCE -DASYNC_IO \
		-DIRIX64 -DSHARED_MEM $(CFLAGS) libasync.c -o libasyncw.o
	$(CC) -32 -O -c  -Dunix -DHAVE_ANSIC_C -D_LARGEFILE64_SOURCE -DASYNC_IO \
		-DIRIX64 -DSHARED_MEM $(CFLAGS) libbif.c -o libbif.o

iozone_IRIX.o:	iozone.c libasync.c libbif.c
	@echo ""
	@echo "Building iozone for IRIX"
	@echo ""
	$(CC)  -O -32 -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO \
		-DNAME='"IRIX"' -DIRIX -DSHARED_MEM $(CFLAGS) iozone.c -o iozone_IRIX.o
	$(CC)  -O -32 -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO \
		-DIRIX -DSHARED_MEM $(CFLAGS) libasync.c -o libasync.o
	$(CC)  -O -32 -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO \
		-DIRIX -DSHARED_MEM $(CFLAGS) libbif.c -o libbif.o

iozone_CrayX1.o: iozone.c libasync.c libbif.c
	@echo ""
	@echo "Building iozone for CrayX1"
	@echo ""
	$(CC)  -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D_LARGEFILE64_SOURCE \
		-DNAME='"CrayX1"' -DIRIX64 -DSHARED_MEM -D__CrayX1__ \
		$(CFLAGS) iozone.c -o iozone_CrayX1.o
	$(CC)  -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D_LARGEFILE64_SOURCE \
		-DIRIX64 -DSHARED_MEM -D__CrayX1__ \
		$(CFLAGS) libasync.c -o libasyncw.o
	$(CC)  -O -c  -Dunix -DHAVE_ANSIC_C -DASYNC_IO -D_LARGEFILE64_SOURCE \
		-DIRIX64 -DSHARED_MEM -D__CrayX1__ $(CFLAGS) libbif.c \
		-o libbif.o

iozone_sppux.o:	iozone.c libbif.c
	@echo ""
	@echo "Building iozone for SPP-UX using Convex compiler"
	@echo ""
	$(NACC) -c  -O -Dunix -D_HPUX_SOURCE -D__convex_spp \
		-DNAME='"sppux"' -Wl,+parallel -DHAVE_ANSIC_C -DHAVE_PREAD \
		-DHAVE_PREADV $(CFLAGS) iozone.c -o iozone_sppux.o
	$(NACC) -O -Dunix -D_HPUX_SOURCE -D__convex_spp \
		-Wl,+parallel -DHAVE_ANSIC_C -DHAVE_PREAD -DHAVE_PREADV \
		$(CFLAGS) libbif.c -o libbif.o

iozone_sppux-10.1.o:	iozone.c libbif.c
	@echo ""
	@echo "Building iozone for SPP-UX using HP ansic compiler"
	@echo ""
	$(NACC) -c -O -Dunix -D_HPUX_SOURCE -D__convex_spp \
		-DHAVE_ANSIC_C -DHAVE_PREAD -DHAVE_PREADV $(CFLAGS) iozone.c \
		 -DNAME='"sppux-10.1"' -Wl,+parallel -o iozone_sppux-10.1.o
	$(NACC) -c -O -Dunix -D_HPUX_SOURCE -D__convex_spp \
		-DHAVE_ANSIC_C -DHAVE_PREAD -DHAVE_PREADV \
		$(CFLAGS) libbif.c -Wl,+parallel -o libbif.o

iozone_sppux_no-10.1.o:	iozone.c libbif.c
	@echo ""
	@echo "Building iozone for SPP-UX no ANSI c compiler"
	@echo ""
	$(CCS) -c -O -Dunix -D_HPUX_SOURCE -D__convex_spp \
		-DNAME='"sppux_no_ansi_10.1"' -Wl,+parallel -DHAVE_PREAD \
		-DHAVE_PREADV $(CFLAGS) iozone.c -o iozone_sppux_no-10.1.o
	$(CCS) -c -O -Dunix -D_HPUX_SOURCE -D__convex_spp \
		-Wl,+parallel -DHAVE_PREAD -DHAVE_PREADV $(CFLAGS) \
		libbif.c -o libbif.o

iozone_convex.o:	iozone.c libbif.c
	@echo ""
	@echo "Building iozone for Convex 'C' series"
	@echo ""
		$(CC) -c -O -Dunix -DNO_THREADS -Dbsd4_2 $(CFLAGS) iozone.c \
			-DNAME='"Convex"' -o iozone_convex.o 
		$(CC) -c -O -Dunix -DNO_THREADS -Dbsd4_2 \
			$(CFLAGS) libbif.c -o libbif.o 

iozone_bsdi.o:	iozone.c libbif.c
	@echo ""
	@echo "Build iozone for BSD/OS"
	@echo ""
	$(CC) -c -O -Dunix -Dbsd4_4 -DHAVE_ANSIC_C \
		-DNAME='"bsdi"' $(CFLAGS) iozone.c -o iozone_bsdi.o
	$(CC) -c -O -Dunix -Dbsd4_4 -DHAVE_ANSIC_C \
		$(CFLAGS) libbif.c -o libbif.o

iozone_freebsd.o:	iozone.c libbif.c libasync.c
	@echo ""
	@echo "Build iozone for FreeBSD"
	@echo ""
	$(CC) -c ${CFLAGS}  -DFreeBSD -Dunix -Dbsd4_4 -DHAVE_ANSIC_C -DASYNC_IO \
		-DHAVE_PREAD -DNAME='"freebsd"' -DSHARED_MEM \
		$(CFLAGS) iozone.c -o iozone_freebsd.o
	$(CC) -c ${CFLAGS} -DFreeBSD -Dunix -Dbsd4_4 -DHAVE_ANSIC_C -DASYNC_IO \
		-DSHARED_MEM -DHAVE_PREAD $(CFLAGS) libbif.c \
		-o libbif.o
	$(CC) -c ${CFLAGS} -DFreeBSD -Dunix -Dbsd4_4 -DHAVE_ANSIC_C -DASYNC_IO \
		-DSHARED_MEM -DHAVE_PREAD $(CFLAGS) libasync.c \
		-o libasync.o

iozone_dragonfly.o:	iozone.c libbif.c
	@echo ""
	@echo "Build iozone for DragonFly"
	@echo ""
	$(CC) -c ${CFLAGS}  -D__DragonFly__ -Dunix -DHAVE_ANSIC_C -DNO_THREADS \
		-DNAME='"dragonfly"' -DSHARED_MEM -DHAVE_PREAD -DHAVE_PREADV \
		$(CFLAGS) iozone.c -o iozone_dragonfly.o
	$(CC) -c ${CFLAGS} -D__DragonFly__ -Dunix -DHAVE_ANSIC_C -DNO_THREADS \
		-DSHARED_MEM -DHAVE_PREAD -DHAVE_PREADV $(CFLAGS) libbif.c \
		-o libbif.o

iozone_macosx.o:	iozone.c libbif.c
	@echo ""
	@echo "Build iozone for MacOSX"
	@echo ""
	$(CC) -c -O -Dunix -Dbsd4_2 -DIOZ_macosx -DHAVE_ANSIC_C -DNO_THREADS \
		-DNAME='"macosx"' -DSHARED_MEM $(CFLAGS) iozone.c -o iozone_macosx.o
	$(CC) -c -O -Dunix -Dbsd4_2 -DIOZ_macosx -DHAVE_ANSIC_C -DNO_THREADS \
		-DSHARED_MEM $(CFLAGS) libbif.c -o libbif.o

iozone_openbsd.o:	iozone.c libbif.c
	@echo ""
	@echo "Build iozone for OpenBSD"
	@echo ""
	$(CC) -c -O -Dunix -Dbsd4_4 -DHAVE_ANSIC_C -DNO_THREADS \
		-DNAME='"openbsd"' -DSHARED_MEM $(CFLAGS) iozone.c -o iozone_openbsd.o
	$(CC) -c -O -Dunix -Dbsd4_4 -DHAVE_ANSIC_C -DNO_THREADS \
		-DSHARED_MEM $(CFLAGS) libbif.c -o libbif.o

iozone_openbsd-threads.o:	iozone.c libbif.c
	@echo ""
	@echo "Build iozone for OpenBSD with threads"
	@echo ""
	$(CC) -c -O -pthread -Dunix -Dbsd4_4 -DHAVE_ANSIC_C \
		-DNAME='"openbsd-threads"' $(CFLAGS) iozone.c \
		-o iozone_openbsd-threads.o
	$(CC) -c -O -pthread -Dunix -Dbsd4_4 -DHAVE_ANSIC_C \
		$(CFLAGS) libbif.c -o libbif.o

iozone_OSFV3.o:	iozone.c libbif.c
	@echo ""
	@echo "Build iozone for OSFV3"
	@echo ""
	$(CC) -O -c -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DOSFV3 \
		-DNAME='"OSFV3"' -DNO_PRINT_LLD -DOSF_64 $(CFLAGS) iozone.c \
		-o iozone_OSFV3.o
	$(CC) -O -c -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DOSFV3 \
		-DNO_PRINT_LLD  -DOSF_64 $(CFLAGS) libbif.c -o libbif.o
	$(CC) -O -c -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DOSFV3 \
		-DNO_PRINT_LLD -DOSF_64 $(CFLAGS) libasync.c -o libasync.o

iozone_OSFV4.o:	iozone.c libbif.c
	@echo ""
	@echo "Build iozone for OSFV4"
	@echo ""
	$(CC) -O -c -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DOSFV4 \
		-DNAME='"OSFV4"' -DNO_PRINT_LLD -DOSF_64 $(CFLAGS) iozone.c \
		-o iozone_OSFV4.o
	$(CC) -O -c -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DOSFV4 \
		-DNO_PRINT_LLD  -DOSF_64 $(CFLAGS) libbif.c -o libbif.o
	$(CC) -O -c -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DOSFV4 \
		-DNO_PRINT_LLD -DOSF_64 $(CFLAGS) libasync.c -o libasync.o

iozone_OSFV5.o:	iozone.c libbif.c
	@echo ""
	@echo "Build iozone for OSFV5"
	@echo ""
	$(CC) -O -c -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DOSFV5 \
		-DNAME='"OSFV5"' -DNO_PRINT_LLD -DOSF_64 $(CFLAGS) iozone.c \
		-o iozone_OSFV5.o
	$(CC) -O -c -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DOSFV5 \
		-DNO_PRINT_LLD  -DOSF_64 $(CFLAGS) libbif.c -o libbif.o
	$(CC) -O -c -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DOSFV5 \
		-DNO_PRINT_LLD -DOSF_64 $(CFLAGS) libasync.c -o libasync.o

iozone_TRU64.o:	iozone.c libbif.c
	@echo ""
	@echo "Build iozone for TRU64"
	@echo ""
	$(CC) -O -c -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DOSFV5 -DTRU64 -DHAVE_PREAD \
		-DNAME='"TRU64"' -DNO_PRINT_LLD -DOSF_64 -pthread $(CFLAGS) iozone.c \
		-o iozone_TRU64.o
	$(CC) -O -c -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DOSFV5 -DHAVE_PREAD \
		-DNO_PRINT_LLD  -DOSF_64 $(CFLAGS) libbif.c -o libbif.o
	$(CC) -O -c -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DOSFV5 -DHAVE_PREAD \
		-DNO_PRINT_LLD -DOSF_64 $(CFLAGS) libasync.c -o libasync.o

iozone_SCO.o:	iozone.c libbif.c
	@echo ""
	@echo "Building iozone SCO "
	@echo ""
	$(GCC) -c -O -DSCO -Dunix -DHAVE_ANSIC_C -DNO_THREADS -DNO_MADVISE \
		-DNAME='"SCO"' $(CFLAGS) iozone.c -o iozone_SCO.o
	$(GCC) -c -O -DSCO -Dunix -DHAVE_ANSIC_C -DNO_THREADS -DNO_MADVISE \
		$(CFLAGS) libbif.c -o libbif.o

iozone_SCO_Unixware_gcc.o:	iozone.c libbif.c libasync.c
	@echo ""
	@echo "Building iozone SCO_Unixware_gcc "
	@echo ""
	$(GCC) -c -O -DSCO_Unixware_gcc -Dunix -DHAVE_ANSIC_C  \
		-DASYNC_IO -D_LARGEFILE64_SOURCE $(CFLAGS) iozone.c  \
		-DNAME='"SCO_Unixware_gcc"' -o iozone_SCO_Unixware_gcc.o
	$(GCC) -c -O -DSCO_Unixware_gcc -Dunix -DHAVE_ANSIC_C  \
		-DASYNC_IO -D_LARGEFILE64_SOURCE $(CFLAGS) libbif.c -o libbif.o
	$(GCC) -c -O -DSCO_Unixware_gcc -Dunix -DHAVE_ANSIC_C  \
		-DASYNC_IO -D_LARGEFILE64_SOURCE $(CFLAGS) libasync.c -o libasync.o

iozone_netbsd.o:	iozone.c libbif.c
	@echo ""
	@echo "Building iozone NetBSD "
	@echo ""
	$(CC) -c -O -Dunix -Dbsd4_4 -DHAVE_ANSIC_C -DNO_THREADS \
		-DNAME='"netbsd"' -DSHARED_MEM $(CFLAGS) iozone.c -o iozone_netbsd.o
	$(CC) -c -O -Dunix -Dbsd4_4 -DHAVE_ANSIC_C -DNO_THREADS \
		-DSHARED_MEM $(CFLAGS) libbif.c -o libbif.o
