.TL
Installing the Plan 9 Distribution
.SH
The software
.PP
The distribution is packaged on four 1.44MB floppy diskettes and one CD-ROM.
Many software packages for PCs can install either from CD-ROM or diskette,
but Plan 9 is different.
The diskettes and the CD-ROM
have
.I different
software on them.
The first diskette is used to install the other versions of the system; it occupies
about 1.4 megabytes of the DOS partition on your hard drive.
The other three diskettes contain a small version of Plan 9
for PCs, intended to let you familiarize yourself with the system; it occupies
a separate 20 megabyte partition at the end of a hard disk, or on an entirely non-DOS disk.
The CD-ROM has the full system, including all the programs and sources
for all the architectures supported by Plan 9.
You will need a 500 megabyte partition or disk to unpack the CD-ROM,
although you may choose to remove pieces of the distribution, such as binaries
for machines you don't have, after installation.
The CD-ROM does not install itself; you must use the first diskette to install it.
.SH
The hardware
.PP
An accompanying document, ``The Various Ports'',
discusses the hardware requirements for the system; you should read
all the sections that describe hardware on which you are planning to run Plan 9.
Throughout this document, we will be talking only about PCs unless other machines
are explicitly mentioned.
.PP
.I
Note: Although Plan 9 runs on a number of different machine architectures,
the installation package is designed to be run on a PC, even when installing
the system for other architectures.
.R
.PP
.I
Note: If you have a copy of the 1993 distribution of the system,
its 9P protocol is incompatible
with this version.
You must install this distribution without using any old Plan 9 systems.
.R
.PP
Plan 9 runs in 32-bit protected mode on i386 or later processors.
It does not use the BIOS, so not all hardware is automatically supported,
only that for which Plan 9 drivers exist.
A summary of the hardware requirements:
An IBM PC clone with an i386, i486, or Pentium processor,
eight megabytes of memory,
ISA or EISA bus,
IDE or SCSI disks,
VGA, and a
3-button serial or PS/2 mouse.
The disk
.CW A:
must be a 3½" floppy drive.
Even within these parameters, though, there are a number of restrictions.
.PP
.I
Make sure you read the section of ``The Various Ports'' describing ``The IBM PC
operating system''
before you try installing the system.
.R
.SH
Ways to install Plan 9
.PP
This section gives an overview of how the installation procedure is arranged;
make sure you read it before proceeding.
.PP
.I
Before installing Plan 9, you must first use diskette 1 to
establish a special installation setup as a set of DOS files in the
directory
.CW C:\eplan9 .
You will then use that software to install the real Plan 9 system.
.R
.PP
Using the installation software,
you have several choices about what sort of Plan 9 environment to build.
Throughout this document, these setups are referred to by number:
.IP 1.
Install the three diskettes (numbered 2, 3, and 4) onto a separate partition on
your hard disk.  This requires 20 megabytes of disk space.
.IP 2.
Install the CD-ROM onto a separate partition on
your hard disk.  This requires 500 megabytes of disk space.
.IP 3.
Use the network
to install a separate stand-alone file server and use it to supply files to other Plan 9
systems (of any type) on the network.
.IP 4.
In a network setup, convert the PC into a CPU server and authentication server, with
its files stored on the remote file server.
.LP
The best way to start is to use step 1 to build a system to use for learning
about the system, even if you plan to do something more ambitious later.
With that system, you can access the CD-ROM for anything else that might
interest you.
Once you're comfortable with that, if you have the disk space
you can use step 2 to build a full Plan 9 system on your PC.
If you have a networked environment or machines other than PCs, you would
probably choose to skip step 2 and use steps 3 and 4 to establish
a full-blown Plan 9 environment.
.PP
The most important thing the small installation
system does is record a description of your hardware
in a form that Plan 9 can read it.
This information is kept in a text file called
.CW plan9.ini
stored on the DOS file system, in the directory
.CW C:\eplan9 .
Familiarize yourself with your hardware configuration before you begin,
so you'll be ready to answer questions about your disks, VGA, and so on.
.SH
Disks
.PP
All four Plan 9 diskettes are in DOS format.
The first is a boot diskette containing
installation software to
configure your system and install
the other three diskettes and/or the CD-ROM.
.PP
Your PC must have DOS installed on the hard disk.
You will also need a separate disk
.I partition
for storing the Plan 9 file system.
DOS has a partition table at the beginning of the
disk and normally uses the whole disk area.
To coexist with DOS, Plan 9 puts its partition table
at the
.I high
end of the disk.
You can avoid repartitioning the disk by installing Plan 9 on a separate disk,
so if you have a spare disk we recommend you use it.
.SH
Finding some disk space for Plan 9
.PP
The first thing you need to do is establish a disk partition to hold the Plan 9 file system.
It must hold
either 20 megabytes (or more) for installation step 1, or 500 megabytes
(or more) for step 2.
Unless you already have a suitable spare partition, which could be all of a spare disk,
you will need to repartition your hard disk.
This must be done carefully because changing the size of an existing partition will
destroy its contents.
If you already know how to repartition, use the method that you are comfortable with.
Otherwise, this section is a thorough explanation of a simple-minded way
to repartition; use this procedure if you're the least bit unsure how DOS
uses its disks.
Even if you already know how to repartition your disk, though,
you should read this
section so you know how Plan 9 is going to use the disk.
.PP
Throughout this section,
the explanation is structured as a description of what's about to happen,
followed by the DOS commands to use and their output.
In other words, don't do any step until you've read the words and the
output of the commands presented here.
.PP
The first step is to back up the DOS system to floppy disks.
Then you will repartition the disk.
Finally, you will restore DOS into its new, smaller partition.
.PP
Our procedure backs up DOS to floppy disks.
The backup will take approximately one floppy for every 1.4 megabytes in use.
If you have a well-loaded system, start with lots of blank floppies.
.PP
Label one diskette
.I PC
.I system
.I disk ,
format it, and copy some useful stuff onto it:
.P1
C:\e> format a: /s
C:\e> cd dos
C:\e> copy format.* a:
C:\e> copy backup.* a:
C:\e> copy restore.* a:
C:\e> copy fdisk.* a:
.P2
.LP
Next, use the
.CW backup
program to copy the DOS system to the floppies.
You'll end up inserting empty floppies one after
another as it asks for them.
As each diskette is finished, add to its label
.I DOS
.I backup
.I n
where
.I n
is the number of the diskette in the series:
.P1
C:\e> backup c: a: /s
.P2
.LP
Now reboot the system with the
.I PC
.I system
.I disk
in drive
.CW A: .
When booting, the PC may ask you to enter date and time;
just press
.CW Enter .
.PP
You are about to repartition
the hard disk by first removing the current
partition and then creating a new smaller one.
Run
.CW FDISK .
It is menu-driven; we present one step per menu:
.P1
A:\e> FDISK
.P2
.LP
Choose
.CW FDISK
option 3, ``Delete DOS Partition'':
.P1
		FDISK Options

1. Create DOS partition or Logical DOS Drive
2. Set active partition
3. Delete DOS partition
4. ...

Enter choice [3]
.P2
.LP
Select the primary partition:
.P1
		Delete DOS Partition or ...

1. Delete Primary DOS Partition
2. Delete Extended DOS Partition
3. ...
4. ...

Enter choice [1]
.P2
.LP
Confirm that you really want to do it,
choose partition 1, and don't enter a label:
.P1 .2i
		Delete Primary DOS Partition

WARNING! Data in the Deleted Primary DOS Partition will be lost
What primary partition do you want to delete..? [1]
Enter Volume Label..............................? [          ]
Are you sure (Y/N)..............................? [Y]
.P2
.LP
Press
.CW Esc
to get back to the top level menu.
.LP
Create a new DOS partition by choosing option 1
on the next menu:
.P1
		FDISK Options

1. Create DOS partition or Logical DOS Drive
2. Set active partition
3. Delete DOS partition
4. ...

Enter choice [1]
.P2
.LP
Create a primary partition by choosing option 1:
.P1
		Create DOS Partition or ...

1. Create Primary DOS Partition
2. Create Extended DOS Partition
3. ...
4. ...

Enter choice [1]
.P2
.PP
Now you need to pick a size for the primary DOS partition.
Since you will restore DOS back into it and will need some free disk space,
you must use a larger size than what you saved on the floppies.
If you just want to install using Plan 9 installation setep 1,
give DOS all but 20 megabytes.
(The 20 megabytes for Plan 9 provides 8 megabytes for files unpacked from the diskettes
and 12 megabytes for new files.)
For setup 2, you'll need 500 megabytes to hold the contents of the CD-ROM.
.PP
To make things concrete, we'll assume you're leaving 39 megabytes for DOS,
but be sure to use the number you've decided for your system:
.P1
		Create Primary DOS Partition

Do you wish to use the maximum available size for a Primary
DOS Partition and make the partition active(Y/N).....?[N]

Enter partition size in Mbytes or percent of disk space (%) to
create a Primary DOS Partition...............[39]
.P2
.LP
Press
.CW Esc
to get back to the top level menu.
Set the partition you
just created to be ``active'':
.P1
		FDISK Options

1. Create DOS partition or Logical DOS Drive
2. Set active partition
3. Delete DOS partition
4. ...

Enter choice [2]
.P2
.LP
then:
.P1 2m
		Set Active Partition

Enter the number of the partition you want to make active...[1]
.P2
.LP
Press
.CW Esc
twice to get out of
.CW FDISK .
The machine will now reboot automatically.
.LP
Now you must format the resized DOS partition to
be a DOS bootable partition; there's no need to label it:
.P1
A:\e> format c: /s

WARNING, ALL DATA ON THE NON-REMOVABLE DISK
DRIVE C: WILL BE LOST!
Proceed with format(Y/N)?y

Volume label (11 characters, ENTER for none)?
.P2
.LP
Now restore your DOS system from the backup floppies.
You will be asked to insert the backup floppies in the same
order as you made them.
This is as exciting as making the backup:
.P1
A> restore a: c: /s
.P2
.SH
Installing diskette 1 on your DOS hard drive
.PP
With disk space established, you're now ready to start installing Plan 9.
The first step is to use diskette 1 to build a special installation-only version
of Plan 9 on the DOS file system on your hard disk.
Insert diskette 1 in the floppy drive
.CW A:
and boot the machine.
If you have problems doing this, now or later, you can also bring DOS up normally
and then switch to the
.CW A:
drive and run
.CW B ,
that is:
.P1
C:\e> A:
A:\e> B
.P2
The program
.CW B ,
also known as
.CW b.com ,
is documented in the manual section
.I b.com (8);
it is the Plan 9 bootstrap program that loads the kernel into memory.
.PP
If your PC has
.I only
SCSI disks, 
you will need to configure diskette 1
for the system to recognize what kind of SCSI controller you have.
The installer will then write that information
.I "on the diskette,
whereupon you can reboot the system and it should behave like a regular PC.
Because the install program must write on the diskette,
you will first need to make a writable copy of it:
the original diskette is write-protected.
The easiest way to do this is with the DOS command:
.P1
DISKCOPY A: A: /V
.P2
.PP
Note: if you have trouble doing any of these steps, you should read the section called
``Read this section if you have trouble installing on the PC'' and follow its
instructions to make a clean DOS boot diskette.
.PP
Once you've booted from diskette 1, the system will automatically load
the install program.  Eventually it will display a line across the top of the screen that reads
.P1
System Installation & Configuration
.P2
It will then put up a menu.
Use the arrow keys to navigate the
various menus as you run this program;
for those without arrow keys,
.CW Tab
also cycles through the menu items.
The
.CW Enter
key selects from the menu.
.PP
If you boot from this disk more than once,
you may be asked if you want to
remove the Plan 9 installation already on the hard disk.
It is always safe to remove that installation and let the install process
install it again.
Another option at this stage is to reconfigure the system,
which is also safe; it just writes a new Plan 9 configuration file.
Later in the installation process, you may return to this stage and use
the reconfiguration option to rewrite the configuration.
The final option, after you have brought up Plan 9 on your hard disk,
is to boot from diskette 1 again and choose the ``File System Installation'' option.
This is necessary to install a network-based Plan 9 setup, as will be
discussed below.
.PP
The install program will tell you what it's doing each step of the way.
First it will ask you to approve copying 1.44 megabytes onto
.CW C: ,
and then put up a scrolling window of the files it's copying.
Once the files are installed, it will ask you to define the configuration
of the devices on your PC.
Use the menu to define which VGA controller and monitor,
mouse port,
Ethernet card if any,
and so on down the list.
For now, ignore the entry that says
.P1
File system console
.P2
If you make a mistake in the definition, you can go back and change anything
from the main configuration menu (the one that starts with
.CW VGA
.CW setup ).
.PP
Once you choose the
.CW Save
.CW configuration
option, the install program will show you the format of the file
it is going to create as the DOS file
.CW C:\eplan9\eplan9.ini .
This file tells Plan 9 what the hardware configuration is.
Inspect it carefully, and if it looks right,
.CW Save
it; if not,
use the
.CW Redo
option on the submenu to return to the configuration menu.
If you try to use an incorrect
.CW plan9.ini
file, the system will not work properly, but you can
return to the ``Reconfigure system'' option on the first screen
of the installation program and create a new
.CW plan9.ini
any time.
If you're confused about what the file should look like or
what hardware parameters there are, look at the manual page
.I plan9.ini (8)
in Volume 1 of the manual and also the ``Various Ports'' document.
If you're unsure of the specifications of your hardware,
you should find most of the information you need in the manuals
supplied by the manufacturers of the devices.
.PP
Once you use the
.CW Save
option to write the
.CW plan9.ini
file to the hard disk, remove the floppy from drive
.CW A:
and follow the directions to reboot DOS.
When you get back to the 
.CW C:\e>\|\|
DOS prompt, type
.P1
PLAN9\eB
.P2
and press
.CW Enter .
If all goes well, the system will bring up a new install program that reads, across
the top of the screen,
.P1
File System Installation
.P2
You are now ready to use the little version of Plan 9 on your
.CW C:
drive to install a real Plan 9 from diskettes 2, 3, and 4 and/or the CD-ROM.
You will see a menu with the various options displayed.
The next few sections go through these options.
The numbered sections refer to items in the top menu.
Other sections provide background to the procedures.
.SH
1. Install 3 Diskette System to local drive.
.PP
In this installation option, you will install the version of the system on floppies 2, 3, and 4
onto a separate Plan 9 partition on a hard disk.
After it is installed, you will still use DOS to bootstrap Plan 9 by the usual method:
typing
.CW PLAN9\eB
to run the version of
.CW b.com
installed on DOS drive
.CW C: .
.PP
The first thing you will need to define is which drive has the partition you've
reserved to hold the Plan 9 file system.
The installer will prompt you with a menu of disks it discovers in your hardware.
When you've chosen, the installer will set up the Plan 9 partition table at the
end of that disk, start up the file system program, and
prompt you to insert diskette 2.
.PP
As the diskettes unpack, a scrolling window will show the files being copied from
the diskette to the new file system.
After all three diskettes have been unpacked,
you will see this menu:
.P1
Make the newly installed Plan 9 the default
Reconfigure the system
Exit to system
Reboot
.P2
Take the first option, which will update your
.CW plan9.ini
to make the system use the newly created file system when you reboot.
After you've done this, the menu will automatically select the
.CW Reboot
option.
Remove the diskette from drive
.CW A:
and use the
.CW Reboot
option on the menu.
.PP
For the record,
the second option on this menu gets you back to the configuration menu;
the third exits to the little Plan 9 on
.CW C: ,
which is only for troubleshooting;
the last will reboot DOS.
.PP
After the reboot, you will be back in DOS and should run
.CW PLAN9\eB .
The system will come up, but not all the way.
It will stop with a line that reads something like this:
.P1
root is from (local, 9600, 19200, il)[local!#H/hd0fs]:
.P2
This is asking you to choose which device to use as the root file system;
you want the default, whatever is in the square brackets, so press
.CW Enter .
Then you will be prompted for a user name:
.P1
user[none]:
.P2
Again, take the default.
The system will turn on the VGA and ask you if it should start the window system.
Say yes,
and you're off.
Click the left mouse button over the window holding text, and you're ready to type commands.
.PP
The next section describes how to establish
a private environment.
.SH
Setting up a stand-alone machine
.PP
If you have set up a PC using option 1 or 2 of the installation menu,
you will have a self-contained Plan 9 environment.
The first thing you need to know about is
the local file system.
Whenever you are running a local Plan 9 file system from hard disk,
a program called
.CW kfs
(see
.I kfs (4))
is providing file service.
It has a set of commands to configure and control the file system; see
.I kfscmd (8)).
Before you reboot, you should run the command
.P1
disk/kfscmd halt
.P2
and wait for the command to complete in order to shut down safely.
Other useful arguments to
.CW disk/kfscmd
are
.CW sync ,
which flushes
.CW kfs 's
buffer cache;
.CW allow ,
which allows anyone to read and write the files; and
.CW disallow ,
which restores normal permission checking.
The
.CW allow
command is necessary to get users established on the file system.
Even if you're planning to be the only person to use the machine,
you should create a user name for yourself.
In particular, don't use
.CW none ,
which has special meaning in Plan 9.
.PP
The permissions checking in kfs is not supposed to be secure, it's just to stop
accidents from escalating into disaster.
Stand-alone Plan 9 file servers have much better security,
and therefore take more work to set up.
We'll return to them in a later section.
.PP
For each user name to be established on the file system, run the command
.P1
/sys/lib/kfsuser username
.P2
This will do all the necessary work to establish a basic Plan 9 environment
for that user.
You might even take a look at the command to see how it works;
it's an
.I rc (1)
script.
After a user name has been added this way, run the command
.P1
disk/kfscmd users
.P2
to cause
.CW kfs
to reread the
.CW /adm/users
file.
.PP
When you reboot and log in using a real user name,
you will be prompted for a password;
just press
.CW Enter
until you have an authentication server set up,
another topic we'll visit later.
.PP
To use the Ethernet, you'll have to add an entry in
.CW /lib/ndb/local
describing your machine.
See the man page
.I ndb (6)
or look at the example for the machine called
.CW term
in the section
of this document called ``Make this PC a CPU or Authentication Server''.
.SH
2. Install CD-ROM to local drive
.PP
This proceeds just like setup option 1, but installs a 500 megabyte file system with
all the software.
You will need to tell the installer which drive holds the CD-ROM.
After that, the sequence is as in option 1.
.PP
Once the CD-ROM is unpacked, which will take several hours,
you will see the top-level menu discussed under option 1;
use it the same way.
.PP
When the system reboots, follow
the same directions as under option 1.
The system should come up the same way, but with many more files visible.
.PP
See the previous section to see how to
get a private environment established now.
.SH
Building a Plan 9 network
.PP
A full Plan 9 environment includes networked file servers,
CPU servers, and authentication servers.
Before setting up such an environment, you should familiarize yourself
with the simple stand-alone system built from the previous installation procedures.
You should also read enough of the manuals to have an idea how the system
is structured.
.PP
None of the networked machines needs to be a PC, but the distribution is packaged
to install
.I from
a PC.
For example, you can use the diskettes to set up a SUN file server, but you must do so
from a PC talking to the SUN over the Ethernet.
.PP
The steps to install a Plan 9 network are as follows:
.IP •
Boot a file server.
.IP •
Use the installation package to load the CD-ROM onto the file server (installation option 3).
.IP •
The file server is now running without authentication.
While it's in this state, edit administrative files holding information about
network addresses, users, and so on.
.IP •
Use the file server unauthenticated for a while to get comfortable with the setup.
.IP •
Turn a PC into a combined CPU server and authentication server
(installation option 4).
.IP •
Enable authentication on the file server.
.SH
3a. Make a PC file server boot disk
.PP
This section tells you how to build a boot diskette to bring up a PC as a file
server.
If your file server is not going to be a PC, ignore this section.
You will, however, need to find a way to get the
file server booted using
.BOOTP
to load the appropriate kernel from the CD-ROM; see
.I booting (8).
.PP
Boot from diskette 1,
and choose the ``File System Installation'' option.
Reboot the machine, start Plan 9 normally, and it will
automatically run the installation software again.
Choose the appropriate option from its top-level menu.
.PP
This installation option writes a new diskette that will boot a Plan 9 PC file server.
To do this, you must have a CD-ROM drive on the machine you are using
to write the diskette, as it will hold files that are only shipped on the CD-ROM.
This also means you must have the
.CW plan9.ini
file set up correctly to access the CD-ROM.
.PP
First you need to select a CD-ROM drive and
load the CD-ROM in the drive.
Then place a writable blank diskette
in drive
.CW A: .
The install program will write the boot diskette.
.PP
At this point, you need to provide the configuration information
.I "for the file server machine.
Walk through the menus as usual, but this time also select the
.P1
File system console
.P2
option.
This defines which device will serve as the administrative console for the file server machine.
You probably want to choose the
.CW CGA
option unless you plan to do something like put a remote terminal on a file server
housed in a machine room.
The
.CW VGA
.CW setup
option is not important, because the system won't use the VGA,
but make sure you define the
.CW Ethernet
and
.CW SCSI
.CW Device
options:
the file server must talk on the Ethernet and the
Plan 9 file sever supports only SCSI disks
(see ``The Various Ports'').
.PP
When the configuration is written to the diskette, try booting the
file server using the diskette as a boot disk.
Read the next section before doing this, so you understand how the
file server comes up: it's a completely different arrangement from a Plan 9 machine
running the normal kernel.
The PC file server behaves much like any other Plan 9 file server.
The one peculiarity is that
it uses the floppy both as a boot disk and as a place to store precious
information stored in non-volatile RAM on other machines; see the
.CW nvr
entry in
.I plan9.ini (8).
.PP
If the diskette has a bad configuration, you can return to the installation
machine and write a new configuration file, iterating until it works.
.SH
Starting up the file server
.PP
.I
Carefully read this section before booting the file server.
You are about to write all over some disks.
.R
Also read
.I fs (8)
and
.I fsconfig (8)
to learn about the commands available on the server console.
.PP
When you have a boot disk for a PC file server,
or have found a way to bootstrap another type of file server,
boot the server machine.
The system will chatter a bit and eventually print
.P1
 ** NVR config checksum is incorrect **
.P2
and enter `config' mode.
If you boot the disk a second time, the checksum
in non-volatile RAM will be correct and the system will say
.P1
for config mode hit a key within 5 seconds
.P2
Do what it says.
.PP
The system will prompt you with
.P1
config:
.P2
To this prompt you must type several lines of text to set up the
configuration of the file system.
The first thing to set is the system's name as known to the network.
If the machine is called
.CW kremvax ,
type
.P1
service kremvax
.P2
and a newline.
Then set the machine's IP address, IP subnet mask, and the IP address of
the gateway by typing (using the correct addresses):
.P1
ip 123.111.112.113
ipgw 123.111.112.1
ipmask 255.255.0.0
.P2
.PP
Now you must establish the disk configuration.  First pick a single
SCSI disk
that will store the configuration information in block 0 of the drive.
This information is held in a block (number zero) not otherwise used by the
file system.  Typically, but not necessarily,
it will live on the same drive as the main file system.
Say that's SCSI unit 4.  Then type
.P1
config w4
.P2
The
.CW w
stands for `wren', Plan 9's generic name for SCSI magnetic disks.
Next you must define the layout of the disks that the machine will use.
Here are a couple of examples; if you can't confidently
extrapolate from them to your setup, see
.I fsconfig (8).
A single SCSI disk on unit 4:
.P1
filsys main w4
.P2
A main file system on unit 4, another on unit 5:
.P1
filsys main w4
filsys other w5
.P2
A single file system spanning units 4 and 5:
.P1
filsys main (w4w5)
.P2
A single file system spanning units 4 and 5, blockwise interleaved:
.P1
filsys main [w4w5]
.P2
One of the file systems you define must be called
.CW main ,
the default file system this machine will serve.
Now initialize the structure on the disks:
.P1
ream main
.P2
(Ream any other file systems you've established, too.)
.PP
Take a careful look at what you've typed.
If it's wrong, reset the machine and start again.
Here's a representative display of what the screen should look like:
.P1
config: service kremvax
config: ip 123.111.112.113
config: ipgw 123.111.112.1
config: ipmask 255.255.0.0
config: config w4
config: filsys main [w4w5]
config: ream main
.P2
.PP
All the configuration commands do is set up data structures; they
do no real work.  Once you leave config mode, the file system will
establish the structure you described.  This means that if you make
a mistake you can reset the machine without doing any damage.
To get out of config mode and on to real work, type
.P1
end
.P2
.PP
After you leave config mode, the file server
will initialize its disk and prompt you:
.P1
kremvax:
.P2
The prompt may get lost among the other output; type a couple of newlines.
.PP
If the system crashes horribly at this point, it's probably because the file server
can't find its disks.
The likeliest reasons are that the configuration was specified
wrong or the NVRAM contains bad information.
If the file server is a PC,
write a new server boot diskette with the correct information; this will also
clear the NVRAM section.
If it's not a PC, reboot the system, enter config mode, and make sure the
configuration is right.
.PP
When the server has prompted you with its service name, type
.P1
users default
.P2
to establish a minimal set of users to own the files unpacked from the disk.
See
.I fs (8)
for more information about such commands.
.PP
Check the time the file server has recorded by typing
.P1
date
.P2
If it's wrong, fix it by adjusting the time in seconds; for example, to advance it an hour:
.P1
date +3600
.P2
We apologize for this interface, but in our lab we get the time from a local network server
so the command is unused.
.PP
You now have a file server that will let you connect and talk to it,
.I "as long as you connect with user name
.CW none .
If you try to connect as someone else, you will be rejected as an `unknown user'.
As
.CW none ,
though, you will be able to install the CD-ROM.
.SH
3b. Install CD-ROM to network file server
.PP
When the file server is up, go back to the installation PC and select this option.
(If you didn't do step 3a, read what it says about getting the installation software running again.)
You will be prompted for the IP addresses of the installation PC,
the gateway's IP address, and
the subnet mask.
The installation must be done with machines on the same subnet,
but the information you provide here will be used to build the
network databases on the file server.
.PP
After specifying the IP information for the PC,
supply the IP address of the file server and its
fully-qualified domain name, e.g.,
.P1
gnu-prep.ai.mit.edu
.P2
.PP
Next the installer will ask you to choose whether to dial the server using
IL or TCP.
You want to use IL; the TCP option is for those trying to set up a UNIX
machine as a Plan 9 file server, an arrangement not covered in this
installation guide (see
.I u9fs (4)).
.PP
The installer will now connect to the server.
.PP
Now load the CD-ROM, tell the installer its configuration,
and you're off.
As when installing to local disk, this will take a few hours.
.SH
A few more facts about the file server
.PP
If you need to reboot the file server
after you have installed the CD-ROM but before you have
authentication enabled, you will need to cheat a little.
The reaming process turns off authentication to make it easy to
establish the administration files, but that state is lost if the system is rebooted.
To restore that state after a reboot,
enter config mode (by typing quickly when prompted after the reboot)
and issue an `allow' command:
.P1
config: allow
.P2
This turns off authentication and permission checking;
don't make a habit of it.
.PP
On SUNs, writing the configuration information
to NVRAM destroys the checksum.  When you eventually reboot, the ROM
will decide its NVRAM is untrustworthy and reset all the values.
You'll need to set them back to what you want.  The ROM will not
touch the Plan 9 configuration information, though.
Here's what we type to get the state back; your state might
be different:
.P1
setenv diag-switch? false
setenv selftest-#megs 0
setenv auto-boot? false
setenv boot-device net
.P2
You'll only need to do this once; afterwards everyone will be happy.
The same problem applies to writing the password; see below.
.SH
4. Make this PC a CPU or Authentication Server
.PP
Before a PC can be turned into a CPU or authentication server,
it must first have a Plan 9 partition table on its disk.
The easiest way to do this is to use installation option 1 (3 diskette) or
2 (CD-ROM) to create a file system on the hard drive.
If you've already installed Plan 9 on this machine,
boot the first diskette and select the ``Reconfigure
system'' option.
Define the configuration of the machine, and on the next menu select the option
to run the build procedure.
When the system reboots, it will come up with the top-level menu of
installation options.
.PP
If you've just done the installation, the installer will already know the
pertinent IP addresses for this machine; if not, you'll have to provide them.
In either case, you'll have to provide the IP address and fully-qualified
domain name of the file server.
The installer will then connect to the server
and then ask you to choose which hard disk to use to store the boot kernel.
A partition named
.CW boot
is used to store the kernel from which a Plan 9 PC boots if the root file
system is not a DOS file system.
For example, if the disk is SCSI unit 0, the partition will be a file named
.CW #w0/sd0boot
or, if the device is bound to
.CW /dev ,
.CW /dev/sd0boot .
The installer is asking you to identify the drive whose boot partition will be written.
.PP
Now you need to choose an `authentication user id', or `authid'.
This is a user name that applies to this authentication domain; it is the user name
that all the servers in that domain will use to identify themselves to
the authentication server; its authentication key is stored on the local machine.
On PCs, the key is stored in a partition called
.CW nvram ;
on other machines, it's stored in real non-volatile RAM.
The same authid must be used on all other file servers, CPU servers,
and authentication servers in this domain.
.PP
After you've defined the authid, the installer will place
.CW /386/9pccpu
in the boot partition.
This is a PC kernel for CPU service with its root file system on a remote server.
.PP
The installer will then ask you to reboot.
Get back to the DOS prompt, then go to the console of the file server.
On its console, type the command
.P1
passwd
.P2
This will prompt you for a password.
This is the password to be used by all servers in the authentication domain; it is
authid's password.
Type the password, confirm it, and then you will be asked to give an authid.
Give the same one as on the CPU server.
Next you will need to give the authentication domain name.
This will typically be the same as your authentication domain name.
If your authid is
.CW eduardo
and your file server is named
.CW kremvax.kremlin.mos.su ,
the file server's screen will now look like this:
.P1
kremvax: passwd
New password:
Confirm password:
Authentication id:eduardo

Authentication domain:kremlin.mos.su
.P2
Finally, still on the file server, type the command
.P1
newuser eduardo
.P2
This adds the authid to the
.CW /adm/users
database file.
The file server now has authentication established.
.PP
Now go back to the CPU server PC and reboot it normally.
You will be prompted for a device from which to get the root file system; use
.CW il .
There will be a pause while the system BOOTPs to find out network information.
Until you get a Plan 9 BOOTP server going, the system will not find what it needs,
and after a few seconds it will give up and ask you.
Type the appropriate information, giving all the relevant IP addresses.
When you are asked for the IP address of the authentication server,
take the default, 0.0.0.0.
This peculiar address tells the kernel to use the local key for authentication
instead of talking to a remote server; since this machine is to be the authentication
server, its local key is the one to use.
After all the IP addresses are specified, the system will come up and ask for a password.
Type the authid password,
.I "the same one you gave on the file server.
The PC should come up and be a CPU server as well as providing authentication
service to other machines.
.PP
You should now install Plan 9, probably just using installation option 1, onto another PC
or get some other machine bootstrapped.
This machine will be a terminal that uses the file server for its files and the authentication
server for authentication.
Then edit the network database
.CW /lib/ndb/local
to add an entry for the terminal.
(Do this on the CPU server, and feel free to run
.CW aux/vga
and
.CW 8½
if you want a nicer environment.)
The entry for a PC named
.CW term
might be:
.P1
ip=123.111.112.7 ether=00608cf2325b sys=term
	dom=term.kremlin.mos.su
	bootf=/386/9pc
	proto=il
.P2
Once the terminal is set up,
make sure its Ethernet is working by booting with
.P1
plan9\eb
.P2
and selecting the root file system to come from
.CW il .
This can be automated later by putting the line
.P1
bootfile=e!0
.P2
in
.CW plan9.ini .
.PP
Now you should add users to the authentication databases.
To install a user, say Ethel,
do the following
.IP •
On the authentication server, type the command
.P1
auth/changeuser ethel
.P2
and provide the requested information.
Ethel will need to provide her password.
This step puts
.CW ethel
in the authentication server's key database.
.IP •
On the file server, type
.P1
newuser ethel
.P2
This puts
.CW ethel
in the file server's list of users and creates the directory
.CW /usr/ethel .
.IP •
Boot the terminal and have Ethel log in there, complete with password.
The system should get the VGA going and then complain about missing files.
Type:
.P1
/sys/lib/newuser
.P2
This will establish a basic profile and start the window system.
The next time Ethel logs in, the window system will start automatically.
.PP
Later, when everything seems solid, you should convert the CPU/authentication
server to boot from local disk.
It will then be able to serve Plan 9 BOOTP requests.
.PP
To keep things simple, this procedure has given the authentication machine the name
.CW p9auth .
If you want to call it something else, wait until everything looks like it's
working and change its name in the file
.CW /lib/ndb/local ;
there will be changes to the entries for the machine itself and
for the authentication server in the
.CW ipnet
entry.
When you're doing that, you can also remove the lines marked
.CW #INSTALL
and
.CW #END ;
they're tags left over from the installation process.
.SH
When it all works
.PP
Sometime soon, check that you can compile the kernels,
change the authid,
and do other administrative things.
The file
.CW /adm/timezone/timezone
contains a description of the default time zone;
if that directory contains a file that describes your time zone, copy it to the default file.
The file
.CW /lib/sky/here
contains the local geographic coordinates, which are certainly
not the same as the location of our espresso machine.
.PP
Read the manual.
Make sure to look at
.I u9fs (4)
to see how to set up Plan 9 access to Unix file systems, and
.I nfsserver (8)
to go the other way.
See the directory
.CW /sys/src/cmd/unix
for some Unix-resident programs whose source is included.
.PP
You have a system!
.SH
Troubleshooting
.PP
This section collects comments about how to work around difficulties in the installation
process.
.PP
If the VGA will not turn on properly,
try using the configuration
.CW monitor=vga
and resolution 640x480x1.
If this works, you may be able to teach the system how to use
a higher-resolution mode.
Reconfigure the system to have no VGA or mouse, or edit
.CW plan9.ini
to remove the lines defining the VGA, mouse, and monitor.
When the system reboots, it will then continue to use CGA
mode, and you will be able to explore the system.
The file
.CW /lib/vgadb
contains descriptions for the various VGA controllers.
See if you can find one that looks close, and add a configuration
line for that controller; these strings look like
.P1
0xC0039="CL-GD540"      # Cirrus Logic BIOS
.P2
and define what string is at what hexadecimal address.
To find such a string, enter
.CW DEBUG
from DOS, dump the BIOS memory starting at
.CW C000:0 ,
and look for an identifying string.
For more information, see
.I vgadb (6)
and
.I vga (8).
.PP
It is possible to examine the CD-ROM from a system other than Plan 9.
The CD-ROM contains a single ISO-9660 file system.
That file system is the complete Plan 9 distribution, structured
as it will be after you install it.
However, since Plan 9 file names do not respect 9660
rules and because 9660 directories do not contain enough
information for our purposes, 
the directories on the disk are extended in a way that only
our software understands.  (ISO-9660 has rules for doing this
safely.)  Thus the structure you see with other software will be
slightly different.  In particular:
.IP
The full status of the file \(em owner, group, permissions, etc. \(em
is not visible to regular 9660 software.
.IP
If the name of the file as stored in 9660, after translation to
lower case, is not the file name as Plan 9 wants it,
a special rule is applied.
The file will have a strictly legal 9660 name,
in the form of either
.CW F000023
for a regular file and
.CW D000314
for a directory.
The Plan 9-specific part of the directory will, however, contain the correct
Plan 9 name.  All such mappings are recorded in the file
.CW /_CONFORM.MAP .
Each line of that file gives the mapping for one file basename,
for example
.P1
F000023      8½.c
.P2
This allows non-Plan 9 software to discover, if desired,
the proper file names.
.PP
When you read the file system from Plan 9, of course,
you see all the files as they are intended to be.
You must therefore use Plan 9 to unpack the distribution.
.SH
Read this section if you have trouble installing on the PC
.PP
As you follow the instructions in the rest of this document, you may encounter
problems such as lockups installing or running the system.
If that happens, the first thing to try is to define a minimal hardware
configuration for Plan 9.
Put diskette 1 in drive
.CW A:
and reboot your machine.
If that doesn't work, bring up DOS normally and then type
.P1
A:B
.P2
to the DOS prompt.
Once the install package is up, use it to define the following hardware configuration
.I only :
.CW VGA
640x480x1,
.CW Monitor
.CW vga ,
and
.CW Mouse
to be the correct port or PS/2.
If your primary DOS partition is on the SCSI,
you must also configure the SCSI controller.
Save that configuration,
take out the diskette from drive
.CW A: ,
and reboot DOS.  Then type
.P1
PLAN9\eB
.P2
to the DOS prompt.
If that works, perform the installation of the 3-diskette system and use it
to experiment with the configuration to get the rest of your hardware going.
.PP
If you still have trouble,
try starting up the system using a fresh DOS boot diskette instead of booting from
your hard drive.
This will create the system environment in which Plan 9 is meant to be installed.
.PP
The following steps will create a DOS boot disk; follow them exactly.
Make sure you understand what these steps do.
Check your DOS manual if you don't understand something.
.IP 1.
First get a blank 1.44MB diskette.
Type
.CW C:
and press
.CW Enter .
.IP 2.
Insert the blank diskette into drive
.CW A: .
.IP 3.
Type
.CW FORMAT
.CW A:/S
and press
.CW Enter .
.IP 4.
You will be prompted to insert a blank diskette into drive
.CW A: .
If you haven't already, insert it now.
When the diskette is in, press
.CW Enter .
.IP 5.
Once the disk has been formatted, you will be asked to label the disk.
Type in a label or press
.CW Enter
for no label.
.IP 6.
You will now be asked whether to format another disk.
Type
.CW N
and press
.CW Enter .
.IP 7.
You must now create a
.CW CONFIG.SYS
file on your new boot disk by typing the following command at the
.CW C:
prompt:
.P1
EDIT A:\eCONFIG.SYS
.P2
and press
.CW Enter .
.IP
When the edit screen appears, type:
.P1
DEVICE=C:\eDOS\eHIMEM.SYS
DOS=HIGH
FILES=30
.P2
.IP 8.
Exit and save the file by pressing:
.P1
.CW ALT-F
.CW X
.CW Y
.P2
.IP 9.
You will  also need an
.CW AUTOEXEC.BAT
file on your new boot disk.
To create one, type:
.P1
EDIT A:\eAUTOEXEC.BAT
.P2
and press
.CW Enter .
When the edit screen appears, type:
.P1
@ECHO OFF
C:
.P2
.IP 10.
Exit and save the file by pressing:
.P1
.CW ALT-F
.CW X
.CW Y
.P2
.PP
You now have a DOS boot diskette.
Insert it into drive
.CW A:
and reboot the PC.
The computer will boot up to the
.CW C:
prompt.
If you have not been able to get Plan 9 diskette 1 to boot, you can now
eject the DOS boot disk and insert Plan 9 diskette 1 into drive
.CW A: .
Then type:
.P1
A:b
.P2
and press
.CW Enter
to boot Plan 9 diskette 1.
If you have already successfully installed Plan 9 onto the
.CW C:
drive, make sure there is no diskette in drive
.CW A:
and type:
.P1
plan9\eb
.P2
and press
.CW Enter
to bring up Plan 9 using the installed Plan 9 system on drive
.CW C: .
