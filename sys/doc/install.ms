.TL
Installing the Plan 9 Distribution
.SH
System requirements
.PP
You need an x86-based PC with 32MB of RAM,
a supported video card, 
and a hard disk with at least 300MB of unpartitioned space
and a free primary partition slot.
(These are requirements for the installation procedure.
A machine with 16MB of memory and no free disk space
at all will work as a terminal.)
.PP
If you wish to install from local media,
you need an extant FAT or EXT2 file system,
or a CD-ROM drive (and a CD writer or some other
way to get the distribution archive on a CD).
If you wish to install over the internet,
you need a supported Ethernet card or
a PPP dial-up account using a modem (not a Winmodem).
.SH
Overview
.PP
The Plan 9 installation takes a number of steps.
First, you must read and agree to the license
and then download a floppy disk image.
You can optionally download the distribution archive
to a local file system at this time.
Once downloaded, the disk image is written
to a disk that you then boot.
The disk boots into an install program, which
lets you create and format the disk partition on
which you will install Plan 9.
If you did not download the distribution archive,
it will be downloaded for you
via an Ethernet or a dialup PPP internet connection, 
and then installed.
After configuring a way to boot your system, 
you're finished.
.PP
These instructions are intended to 
be complete enough to carry out a 
typical install, without needing any of
the manual pages for the commands involved.
Of course, the manual pages are still
a more complete reference;
in particular, see
.I prep (8) 
for information on more complex
ways to manipulate the disk partitions.
.SH
Before you begin
.PP
\f4Back up anything you care about on your hard disk!\f1
We have taken great pains to ensure that installing
Plan 9 will not cause the loss of other data on your disk,
but as with any operating system installation, there are
times when you will be editing disk partition tables,
and you might make a mistake, or there might
be a bug somewhere in our install software.
You are installing this software at your own risk.
.PP
.SH
Obtaining the boot disk
.PP
\f4Go to the web page
._H <a href="http://plan9.bell-labs.com/plan9dist">
http://plan9.bell-labs.com/plan9dist
._H </a>
and read and agree to the license terms.\fP
.PP
\f4Describe your machine.\fP
You need to select the monitor, video resolution, and mouse type
that will be used during the installation.
If you want to install over an Ethernet connection,
you need to select an Ethernet card.
If you do not find your monitor in the list, one
of the multisyncs running at 640x480x8 or 800x600x8
is likely to work
if you are not using an LCD.
If you find you cannot boot the resulting boot disk,
your hardware is likely not supported.
.PP
\f4Download and write the disk.\fP
After you have described your machine,
download the boot disk (called
.CW 9disk.9fd )
from the web page.
At this point, if you wish to install off local media,
you can also download the distribution archive to
a local FAT (DOS, Windows) or EXT2 (Linux)
file system.
Remember where you put it,
and don't change its name
.CW plan9.9gz ). (
.PP
Using a utility such as
._H <a href="ftp://ftp.freebsd.org/pub/FreeBSD/tools/rawrite.exe">
.CW rawrite
._H </a>
under DOS
(or
.CW cat
under a Unix variant),
write the boot disk image (the file
.CW 9disk.9fd )
to a formatted floppy disk.
Do 
.I not
write-protect the floppy disk.
.PP
\f4Cold boot your machine from the floppy disk.\fP
That is, power down,
and power it back up rather than restarting
from inside Windows or using the reset button.
In a few cases (especially on laptops) the drivers
get cranky when not starting from a cold boot.
At the
.CW root
.CW is
.CW from
prompt, press enter.
Booting may take a few minutes, depending on the
speed of your floppy drive.
If you find yourself at a 
.CW boot
.CW from:
prompt, it is likely that the bootstrap
program has not detected your floppy drive;
note the devices listed on the
.CW boot
.CW devices:
line (which should be just above the 
.CW boot
.CW from:
line), and jump to the troubleshooting section.
.PP
\f4Your screen should switch into graphics mode
shortly after the floppy light goes off.\fP
If it does not, or you find yourself at a
.CW % 
prompt, jump to the troubleshooting section at
the end of this document.
.SH
The installation process
.PP
Once you have booted from the floppy disk, 
you should find yourself looking at a grey
screen with some windows.
Interaction with the installation program is textual.
The large upper window contains the
install process itself.
The window under it is a running log
of what has happened.
A statistics graph is in the bottom corner;
if you're curious,
the graphs, from top to bottom, show
system load, memory usage, interrupt rate,
system call rate, context switch rate,
and ethernet packet rate.
.PP
You are running the new Plan 9 window system,
.CW rio .
Unless you have prior experience with
Unix's 
.CW mux
or
Plan 9's
.CW 8½ ,
the window system is likely to be unfamiliar.
For the purposes of the install process,
you need not interact with the window system proper,
so \f4simply do not touch the mouse.\fP
On systems with very small screens, you
may find it necessary to use the keyboard's arrow
keys to scroll the window up and down.
.PP
Installation is short.
First, you select a disk and create
and initialize
a Plan 9 partition.
Then the distribution archives are
located (or downloaded if necessary)
and installed.
Finally, you create a boot disk or
prepare some other way to boot the system.
.PP
At many points, you will be prompted to
provide some information (say, an IP address)
or select from a list of choices (say, the disk
to use).
The prompts will be of the form
.P1
Your choice (foo, bar, baz, quux)[quux]:
.P2
When there are a small fixed list of possible
answers, they will be listed in parentheses
as in 
.CW "(foo, bar, baz, quux)" .
If there is a default choice, it will
be in square brackets as in 
.CW [quux] .
Simply pressing enter at such a prompt
selects the default.
.PP
The installation is broken down into 
a number of things to do (select a disk,
download the archives, etc.), some
of which depend on others.
The heart of the installation program 
is a screen listing which parts you have
accomplished, and which parts you can do next.
For example:
.P1
The following tasks are done: 
  pickdisk    - select disk to use for plan 9 file system
  partdisk    - create plan 9 partition
  prepdisk    - subdivide plan 9 partition
  reamfs      - initialize plan 9 file system
  mountfs     - mount the plan 9 file system

The following unfinished tasks are ready to be done:
  configarch  - configure source of distribution archives
  stop        - save current state of install, to be resumed later

Task to do [configarch]: 
.P2
A default will be provided, so
pressing enter at the prompt will move
the installation along.
At the same time, you can select to 
repeat a task to change your choices
(for example, to select a different disk).
Typing <control-d>
at any prompt will abort the current task
and return you to the main screen.
.PP
If you wish to stop the install process
at any time, you can choose
.CW stop ,
which will save the state of the current
installation to the boot floppy.
When you are ready to continue, you can
simply boot the floppy again.
.SH
Choosing a disk (pickdisk)
.PP
You need to select a disk on which to install Plan 9.
The current partitioning scheme
is incompatible with the scheme used by
the 1995 and earlier releases,
so you should not try to use an extant
old Plan 9 disk as your install disk,
unless you don't mind trashing your current
installation.
(While the installation creates a new-style
disk, the old disk format is recognized 
by the boot program and kernels, and the
.CW kfs
file system format has not changed, so
old disks are still legible in the new system.
If you have an old Plan 9 disk with data
you want to save, we suggest you install
to a clean disk and then copy the data over.)
.PP
The install process will scan your
disk devices and give you a list of them, 
along with manufacturer identification strings and 
disk partition tables.  For example, you might see:
.\" rsc: new figure for next distribution (with new fdisk)
.\" * p1                    0 255        (255 cylinders, 1.95GB) FATHUGE
.\"   p3                  510 784        (274 cylinders, 2.09GB) LINUX
.P1
The following disk devices were found.

sdC0 - WDC AC36400L
 * p1                   63 4096575    (4096512 sectors, 1.95 GB) FATHUGE
   p3              8193150 12594960   (4401810 sectors, 2.09 GB) LINUX
empty partitions:  p2 p4

sdD0 - IDE-CD ReWritable-2x2x63.014VO07982013140700210

Disk to use (sdC0, sdD0):
.P2
The Plan 9 names for storage devices are
of the form
.CW sdXX .
The names
.CW sdC0
and
.CW sdC1
are the master and slave on the primary ATA controller,
.CW sdD0
and
.CW sdD1
are on the secondary, 
and 
.CW sdE0 ,
.CW sdE1 ,
.CW sdF0 ,
and
.CW sdF1
are on additional ATA cards.
SCSI devices are named
.CW sd\fINT\fP ,
where 
.I N
is the SCSI controller number,
and
.I T
the SCSI target number.
.PP
\f4Choose a disk on which to install Plan 9.\fP
It should have at least 300MB of empty
disk space.
If you will be installing TeX on the system
(see end)
you'll need another 100MB.
In order to have enough room for a swap 
partition and some room to play, at least 500MB to a gigabyte is suggested.
The disk on which you install also needs
a spare entry for the Plan 9 partition
in the primary partition table.
That is, if the disk description says
.CW empty "" ``
.CW partitions:
.CW none '',
you cannot install Plan 9 on it
(unless you delete an extant partition).
.SH
Creating a Plan 9 partition (partdisk)
.PP
Once you have chosen the disk, you will
need to create a Plan 9 partition.
To do this, the install process will run
the Plan 9 
.CW fdisk
program and let you partition the disk.
If the disk does not already have a Plan 9 
partition, 
.CW fdisk
will suggest one by creating (but not writing)
a partition in the largest contiguous empty space
it can find.
For example, you might see:
.P1
.\" rsc: new figure for new fdisk
.\"  * p1                    0 255        (255 sectors, 1.95 GB) FATHUGE
.\" '  p2                  255 510        (255 sectors, 1.95 GB) PLAN9
.\"    p3                  510 784        (274 sectors, 2.09 GB) LINUX
    mbr                   0 63         (63 sectors, 31.25 KB) EMPTY
  * p1                   63 4096575    (4096512 sectors, 1.95 GB) FATHUGE
 '  p2              4096575 8193150    (4096575 sectors, 1.95 GB) PLAN9
    p3              8193150 12594960   (4401810 sectors, 2.09 GB) LINUX
 >>>
.P2
Each line contains a partition name
.CW p1 , (
.CW p2 ,
.CW p3 ,
and
.CW p4
are the only valid names),
the starting and ending cylinder, the size of the partition, 
and the type of partition.
Note that partitions include the starting
cylinder but not the ending cylinder.
.CW >>>
is the prompt.
In this example, the
.CW *
next to
.CW p1
means that
.CW p1
is the active partition (i.e. the one used when booting from the disk),
and the
.CW '
next to
.CW p2
means that the partition table
entry for
.CW p2
is different from what is on the disk; that is, changes
have been made but not written.
In this example,
.CW fdisk
has created
.CW p2
in what was previously unpartitioned space.
.PP
\f4Create the Plan 9 partition and quit fdisk\fP.
If you agree with
.CW fdisk 's
proposal, you need only type
.CW w
to write the changes
and then
.CW q
to quit 
.CW fdisk .
Otherwise, you can edit the table
yourself, using the
.CW "a\fI \fPp\fIN\fP
and
.CW "d\fI \fPp\fIN\fP
commands to add and delete partitions.
.SH
Partitioning the Plan 9 partition (prepdisk)
.PP
The Plan 9 partition you just created is
typically further subdivided into a small
FAT configuration partition 
.CW 9fat ), (
a Plan 9 file system partition
.CW fs ), (
a swap partition
.CW swap ), (
and sometimes a network disk cache partition
.CW cfs ). (
Once you have created the Plan 9 partition,
the install process will run 
.CW prep ,
which is used to partition the Plan 9 partition.
The interface is very similar to
.CW fdisk 's;
the main differences are unrestricted names\",
.\" rsc: the use of sectors rather than cylinders as the allocation unit,
and the lack of an active partition and partition types.
Just as 
.CW fdisk
did,
.CW prep
will, if there are no partitions already in place,
create a suggested layout.
.PP
\f4Create 9fat, fs, and swap partitions and quit prep.\fP
The 
.CW 9fat
partition must be located first within the Plan 9 partition.
You do not need a lot of swap space \(em in normal usage
Plan 9 terminals do not swap.  A swap partition half
the size of your main memory is probably more than enough.
As with
.CW fdisk ,
if you find the layout satisfactory you can
simply write it and quit; otherwise
you can edit it yourself.
.SH
Intializing the kfs file system (reamfs, mountfs)
.PP
Once you have partitioned your disk,
the install process will initialize and mount a new
.CW kfs
file system in the newly created
.CW fs 
partition.
.SH
Locating the distribution on local disk (configarch)
.PP
At this point, you need to tell the install process
where to find the distribution archives.
If you downloaded the archives earlier,
you will need to specify the disk partition on which
they reside, along with the path name.
.PP
\f4Choose the partition containing the archives.\fP
You will be shown a list of disk partitions
that the install process can read.
.PP
\f4Choose the directory containing plan9.9gz.\fP
Once you have chosen a partition, 
you will need to give the name of
the directory on that partition which
contains the distribution archives.
If you type 
.CW browse
instead of a directory name, you
will be dropped into a minimal shell
that you can use to find the files.
Specifically, the shell has three
commands: 
.CW cd
.I dir
changes directories,
.CW lc
prints a columned list of files in the current directory, and
.CW exit
exits.
Once you are in the directory
containing the archive
(or if you give up the search),
simply exit the shell.
.PP
If you have found the distribution
on local disk, you may skip
the next section and go to ``Unpacking the distribution''.
.SH
Locating the distribution on the network (configarch)
.PP
\f4Configure your internet connection.\fP
The install process will scan for devices:
if both a serial port and an Ethernet card
are found, you will be prompted whether to use
Ethernet or PPP; otherwise the device that is found
will be used.
.PP
If you are using Ethernet, you can enter
your IP configuration manually or via DHCP.
If you choose to enter the configuration
manually, be sure to have your
IP address, network mask, and gateway IP address.
.PP
If you are using PPP, you will have to choose
a serial device and connection method.
You can choose to dial and log yourself in or to have
Plan 9 dial and use CHAP to log in (this is how the
stock Windows PPP client connects, for example).
.PP
If you choose to log yourself in, 
you will be dropped into a conversation
with the modem.
Dial, log in, and once PPP has started,
type <control-d>.
You may need to type <control-m> rather
than <enter> to get a response from the modem.
.PP
If you use CHAP, the install process will
prompt for a phone number (exactly as you
would dial it yourself, with any necessary
prefixes; numbers only), user name, and password.
It will then dial and initialize the connection.
.SH
Fetching the distribution from the network (download)
.PP
Once the network is started, you can begin
(or continue) downloading the distribution archive.
If the last download was interrupted (e.g., the network
connection was lost or you pressed the Delete key), it will
begin where it left off.
.PP
Once the download is complete, you may wish to 
run the task
.CW stopppp
to hang up your PPP
connection.
Similarly,
.CW stopether
will deactivate your Ethernet connection
(but seems less useful).
.SH
Unpacking the distribution
.PP
Once the archive has been located or downloaded,
selecting
.CW unpack
will extract the distribution archive
to the newly created
.CW kfs
file system.
The log window will display the name and
size of each file as it is extracted.
This takes about 45 minutes when installing
from a local disk.
.SH
Preparing to boot the new installation (bootsetup)
.PP
The first time you run
.CW bootsetup ,
it initializes the
.CW 9fat
configuration partition with appropriate
bootstrap code as well as a modified
version of your
.CW plan9.ini
from the boot floppy, and a 
.CW 9pcdisk
kernel.
.PP
In order to boot into Plan 9, another
bootstrap program must locate this partition,
read 
.CW plan9.ini ,
and boot the kernel.
There are a number of ways to make this happen,
all selectable from the
.CW bootsetup
menu.  If you wish to use more than one
method, simply run
.CW bootsetup
multiple times.
.IP \f(CWfloppy\fP\ 
Create a boot floppy.
In addition to a bootstrap program,
the floppy will contain a kernel and a backup of your
.CW plan9.ini
file named
.CW plan9ini.bak ,
but will not use them.
Instead, the floppy will load
.CW plan9.ini 
and the kernel from your
.CW 9fat
partition.
To boot the kernel on the floppy
(useful as a rescue mechanism if you trash your
.CW 9fat
partition), copy
.CW plan9ini.bak
to
.CW plan9.ini
and change the line
.CW bootfile=sdXX!9fat!9pcdisk
to
.CW bootfile=fd0!9pcdisk.gz .
.IP \f(CWwin9x\fP\ 
Edit the Windows startup menu to list
Plan 9 as an option.
Your
.CW c:\econfig.sys
and
.CW c:\eautoexec.bat
files will be saved as
.CW config.p9
and
.CW autoexec.p9 ,
and then edited.
A bootstrap program as well as
.CW plan9ini.bak
and a kernel will be copied to
the directory
.CW c:\eplan9
(created if necessary).
The procedure described above for
rescue works here too, but the bootfile
should become
.CW sdC0!dos!plan9/9pcdisk .
.IP \f(CWwinnt\fP\ 
Edit the Windows NT boot menu to list
Plan 9 as an option.
This is only possible when your ``c:'' drive
is a FAT partition, since the boot configuration must be accessible.
Your
.CW c:\eboot.ini
file will be saved as
.CW boot.p9 ,
and then edited.
This will also create the file
.CW c:\ebootsect.p9 ,
which the NT boot manager will use to load Plan 9.
.IP \f(CWplan9\fP\ 
Set the Plan 9 partition to be the active one
(i.e. the partition booted by default).
This is only useful if you have installed Plan 9
on your first hard disk.
You can always set another partition active later
by using
.CW disk/fdisk .
.SH
Pausing and resuming the installation (stop)
.PP
If you need to stop the installation process
and come back to it later,
you can run the
.CW stop
task,
which will save the state of the installation
to the file
.CW 9inst.cnf
on the boot floppy and then let you
turn off your system.
Booting again from this floppy will
pick up where you left off, with two exceptions.
.PP
First, if you have already initialized the destination
file system, you will have to run the
.CW mountfs
task to remount it.
.PP
Second, if you have chosen to use Ethernet or PPP
to obtain the archives but have
not completed the download, you will need to
explicitly run the 
.CW startether
or
.CW startppp
task to reestablish the connection.
.SH
The end (finish)
.PP
Choosing the 
.CW finish
task will halt the
.CW kfs 
file system and print a message saying it
is safe to reboot your computer.
.PP
\f4Note: If your hard disk has a copy of the 1995 Plan 9 distribution, before
booting the new distribution, rename or hide the old plan9.ini to
avoid confusing the new installation.\f1
.SH
Setting up
.PP
The first time you bring up the new system, log in as
.CW glenda .
You don't need to type a password, just type enter.
.PP
Once the system's up you'll need to set up a few things.
First, have a look at
.I "Getting Started With Plan 9" ,
in
.CW /sys/doc/start.ms
or
._H <a href="http://plan9.bell-labs.com/sys/doc/start.html">
.CW http://plan9.bell-labs.com/sys/doc/start.html .
._H </a>
This will tell you how to do some things, such as adding the real users.
.PP
Many aspects of the system are configured by environment variables.
Very early, you will want to make changes to
.CW /rc/bin/termrc
and
.CW /rc/bin/cpurc
to set up appropriate values for
.CW $site
(the local mail domain),
.CW $fileserver
(the local domain name of your networked file server, if you have one; it's shipped with value
.CW kfs ,
which points to the local disk-resident file system),
.CW $cpu
(the local domain name of your main CPU server), and
.CW $facedom
(the special name used in the faces database to identify your colleagues).
You will also want to test that VGA hardware
acceleration works, and if so comment out
the line
in
.CW /rc/bin/termrc
that disables it:
.P1
# turn off hardware acceleration until you know it's ok.
echo -n 'hwaccel off' >'#v/vgactl' >[2]/dev/null
.P2
If you edited 
.CW vgadb
on the floppy disk or used a different 
.CW aux/vga
to get the install program going,
you'll have to copy them over manually
to the new distribution.
Once it boots and rio fails, put the install disk
in and copy them over:
.P1
a:
disk/kfscmd allow
cp /n/a:/vgadb /lib/vgadb
cp /n/a:/vga /386/bin/aux/vga  # if you used a new vga
disk/kfscmd disallow
disk/kfscmd halt
.P2
And reboot by typing Ctl-Alt-Del.
Rebooting isn't strictly necessary, but it's
the surest way to test that the system
will come up right the next time you boot.
.PP
When shutting down your system,
you should first have the 
.I kfs
file system sync and halt the disk, by
typing
.P1
disk/kfscmd halt
.P2
and then waiting for the message
.CW kfs: "" ``
.CW file
.CW system
.CW halted ''.
For more such information, see the
.I "Getting Started
document.
.SH
Troubleshooting
.PP
The initial bootstrap of a new operating system on new hardware is often
problematic.  Here follow some suggestions that might help
you clear some hurdles.
We have set up a trouble line\(emmail to
.CW 9trouble@plan9.bell-labs.com \(em
but please try the following ideas before contacting us.
Also the Plan 9 Usenet group,
.CW comp.os.plan9 ,
may be a source of help.
.PP
A up-to-date list of encountered problems and their solutions can be found at
._H <a href="http://plan9.bell-labs.com/plan9dist/errata.html">
.CW http://plan9.bell-labs.com/plan9dist/errata.html .
._H </a>
.PP
The most common problems will be in the connection between hardware and software.
First, make sure that the hardware you're running is supported by the drivers
in the distribution.
Check the list in
._H <a href="http://plan9.bell-labs.com/plan9dist/pchardware.html">
.CW http://plan9.bell-labs.com/plan9dist/pchardware.html .
._H </a>
.PP
Your
.CW plan9.ini
file must be an accurate description of the machine.
The first line of defense is therefore to look at the
.CW plan9.ini
file and edit it.
The floppy is a DOS floppy, so you should be able to edit
.CW plan9.ini
from a Windows machine or other system.
.PP
.CW 9load
(see
.I 9load (8))
is the program that loads and starts the kernel.
It needs to find the kernel, based on
.CW plan9.ini
and the hardware it can discover,
and copy it into memory.
If
.CW 9load
has trouble,
you can turn on debugging by typing a control-R at any time while it's running.
Even if the debugging output doesn't help you, what's printed might
help others, so make notes.
.PP
If the kernel hangs after printing
.CW time... ,
something in the startup scripts has failed.
To see each command before it is executed,
add the line
.CW debug=1
to the file
.CW plan9.ini
in the root directory of the boot floppy.
If the kernel reboots before you get a chance
to read what is on the screen, you might
try attaching a serial console and adding
the line
.CW console=0
or
.CW console=1
to send kernel output to DOS's COM1 or COM2
as well as the screen.
.PP
If the kernel gets running but the VGA doesn't turn on
you may need to play with the settings.
If the screen goes black and you see nothing,
.CW aux/vga
thinks it recognizes your video card, but either
the monitor settings being used are incorrect or
.CW aux/vga
doesn't really know everything it needs to program your card.
In this case you might try a smaller screen resolution, starting
at 
.CW 640x480x8
and working up.
A
.CW 640x480
screen is perfectly adequate for the installation.
If you are using an LCD, you should use the exact
size of the LCD; 
.CW aux/vga
sometimes has problems stretching
smaller resolutions on LCDs.
If the kernel doesn't switch into VGA mode but continues to run in CGA mode,
along with a complaint about
.CW rio
panicking and a shell prompt
.CW % ), (
the system doesn't recognize your video card at all.
.CW Aux/vga
will have left a hex dump of your VGA BIOS memory
on the screen.
The problem might be as simple as adding
a line to
.CW /lib/vgadb ,
or the card might not be supported.
Look through the strings in the memory
for a chipset type and check the hardware list.
Beware, though, that if you don't know exactly what sort of chip you have,
editing
.CW /lib/vgadb
could be dangerous.
If you can find out the exact chip type\(emsuch as by looking in the hardware manual,
the Display Properties in Windows 95, 98, or NT, or the configuration information used
by a Unix-like system\(emsee if
.CW /lib/vgadb
supports it.
If not, you may need to go the last resort mentioned at the end of this section.
.PP
If you have other video cards, it can't hurt
to try a different one.
.PP
Before invoking
.CW aux/vga
to start the VGA, the floppy boot script
writes the output of
.CW aux/vga
.CW -vip
to the file
.CW vgainfo.txt
in the root directory of the floppy disk.
The boot disk uses the 
.CW vgadb
file from the root directory of the floppy disk as
.CW /lib/vgadb ,
to make it possible to edit on
other systems.
.PP
See 
.I vga (3),
.I vgadb (6),
and
.I vga (8)
for more information.
Sometimes it suffices to add some information to
.CW /lib/vgadb ;
if you find this to be true, please let us know so we can update our master database.
.PP
One note: the Plan 9 kernel now depends on 8 or more bits per pixel on the display,
which means the standard VGA modes can't be relied on.
The system really does need to know how to turn on
some special settings in the VGA controller.
.PP
As a last resort, ask the experts in
.CW comp.os.plan9
or mail
.CW 9trouble@plan9.bell-labs.com .
If you mail
.CW 9trouble ,
please include the
contents of both
.CW plan9.ini
and
.CW vgainfo.txt
from the boot floppy,
as well as any hardware information gleaned from other sources.
