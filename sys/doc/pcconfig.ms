.TL
Configuring a PC
.AU
Dave Presotto
.PP
The Plan 9 distribution comes with an image for a 1.4 megabyte
floppy disk, stored in the file
.CW /sys/lib/pcdisk .
This floppy can be used as a stand alone Plan 9 system that
uses the DOS file system on the floppy as its own file system.
It can also be used to prepare a DOS hard disk partition as
a Plan 9 file system or,
if you already have a plan 9 file server running, you
can just boot Plan 9 from the floppy and attach directly to
the file server using it as your root file system, see
.I booting (8).
.PP
The PC comes with DOS installed.
These instructions take you from a 386 or 486 PC with DOS
to a PC with hard disk partitions for both DOS and Plan 9.
The DOS-related steps vary with different versions of
DOS and its format and fdisk programs.
Look before you leap.
.SH 1
Backing up DOS
.PP
Start by backing up the DOS system to floppies.
After repartitioning the disk you will restore this system into a smaller
area.
.IP 1.
Find at least 13 empty floppies and label them all with the ID of the
PC you are about to convert.
.IP 2.
Turn on the PC
.IP 3.
Add to the label of one floppy
.I "PC system disk" .
Plug in an empty floppy, format it, and copy some useful stuff onto it.
.P1
C> format a: /s
C> cd dos
C> copy format.* a:
C> copy backup.* a:
C> copy restore.* a:
C> copy fdisk.* a:
.P2
.IP 4.
Backup the DOS system to floppies.  You'll end up inserting empty floppies one after
another as it asks for them.  As each floppy is finished add to its label
DOS backup
.I n
where
.I n
is the number of the floppy in the dump.
.P1
C> backup c: a: /s
.P2
.PP
Now reboot the system with the system floppy plugged in.
When booting the PC may ask you to enter date and time.
Just hit return.
.PP
You are about to repartition
the hard disk by first removing the current
partition and then creating a new smaller one.
.IP 5.
Run FDISK.  It is menu driven.  We present 1 step per menu.
.P1
A> FDISK
.P2
.IP 6.
Choose FDISK option 3, delete DOS partition.
.P1
		FDISK Options

1. Create DOS partition or Logical DOS Drive
2. Set active partition
3. Delete DOS Partition
4. ...

.P2
Enter choice [3]
.IP 7.
Select the primary partition.
.P1
		Delete DOS Partition or ...

1. Delete Primary DOS Partition
2. Delete extended DOS Partition
3. ...
4. ...

.P2
Enter choice [1]
.IP 8.
Confirm that you really want to do it.
Choose partition 1.  Don't enter a label.
.P1
		Delete Primary DOS Partition

WARNING! Data in the Deleted Primary DOS Partition will be lost
What primary partition di you want to delete..? [1]
Enter Volume Label..............................? [             ]
Are you sure (Y/N)..............................? [Y]
.P2
.IP 9.
Type escape to get back to top level menu.
.IP 10.
Create a new DOS partition.  Choose 1.
.P1
		FDISK Options

1. Create DOS partition or Logical DOS Drive
2. Set active partition
3. Delete DOS Partition
4. ...

.P2
Enter choice [1]
.IP 11
Create a primary partition.  Choose 1.
.P1
		Create DOS Partition or ...

1. Create Primary DOS Partition
2. Create extended DOS Partition
3. ...
4. ...

Enter choice [1]
.P2
.IP 12
Pick a size for the primary partition.
We normally use 19 megabytes, not the maximum.
.P1
		Create Primary DOS Partition

Do you wish to use the maximum available size for a Primary DOS Partition
and make the partition active(Y/N).....?[N]

Enter partition size in Mbytes or percent of disk space (%) to
create a Primary DOS Partition...............[19]
.P2
.IP 13.
Type escape to get back to top level menu.
.IP 14.
Set the partition you just created to active.  Choose 2.
.P1
		FDISK Options

1. Create DOS partition or Logical DOS Drive
2. Set active partition
3. Delete DOS Partition
4. ...

.P2
Enter choice [2]
.IP 15.
Tell it partition 1.
.P1
		Set Active Partition

Enter the number of the partition you want to make active.....[1]
.P2
.IP 16.
Type escape twice to get out of FDISK.  The system will now reboot.
.PP
We can now format the partition we just created and restore the
DOS system to it.
.IP 17.
Format the hard disk.
Don't give it a label.
.P1
A> format c: /s

WARNING, ALL DATA ON THE NON-REMOVABLE DISK
DRIVE C: WILL BE LOST!
Proceed with format(Y/N)?y

Volume label (11 characters, ENTER for none)?
.P2
.IP 18.
Restore the backup.  You will be asked to insert the backup floppies in the same
order as you made them.  This is almost as exciting as making the
backup.
.P1
A> restore a: c: /s
.P2
.SH 1
Installing Plan 9
.PP
You now have a hole at the end of the hard disk to make Plan 9 partitions.
First copy the Plan 9 floppy image
.CW /sys/lib/pcdisk ) (
onto a diskette somehow.
If you can't do that, copy the directory tree starting at
.CW /sys/lib/pc
onto a floppy.
.IP 20.
Reboot the system with no floppy in it.  You can do this by
cycling the power.
.IP 21.
Insert Plan 9 boot floppy and if there is another floppy drive, make sure
it is empty.
Run the Plan 9 boot program
.P1
C> a:b.com
.P2
The boot program will ask you where to boot from.  Take the default:
type return.
.IP 22.
Plan 9 will now boot.
Tell it to connect using the local floppy as a file system.
Then login as anyone.
It'll believe any 8 character password.
.P1
root is from (tcp, il, local)[local]: l
user[none]:yournamehere
password:
.P2
.IP 24.
The system will come up and try to mount both the floppy and
hard disks.
There may be some error messages related to mounting a second
floppy.
Eventually you will get a prompt.
.P1
% 
.P2
You now have a running Plan 9.
You can either play with it or start configuring file systems.
.IP 25.
To configure Plan 9 partitions on the hard disk, run the program
.I disk/prep (8).
It will ask you partition info and write a Plan 9 partition
sector on the last sector of the disk.
If it finds a DOS active partition on the disk, it will
tell you.
All numbers it asks for are in 512 byte sectors.
At minimum, you will want a boot partition called "boot",
a swap partition called "swap", and a file system partition
called "fs".
The boot partition should be bigger than the kernel since
that's what is copied into it.
.P1
% disk/prep /dev/hd0
sector = 512 bytes, disk = 205561 sectors
DOS partition table exists

Nr Type           Start      Len
1  Huge           23         100000

(d)elete, (e)dit or (q)uit ? e


Nr Name                 Overlap       Start      End        %     Size
 0 /hd0disk             0123456           0   205561      100   205561
 1 /hd0partition        01-----      205560   205561        0        1

 2 name (dos [- to delete, * to quit]):

.P2
.IP 26.
Take the defaults for the DOS partition.
.P1

 2 name (dos [- to delete, * to quit]):
 2 start (0):
 2 length (100023):
.P2
.IP 27.
Now create your own partitions.
.P1
 3 name ( [- to delete, * to quit]): boot
 3 start (100023):
 3 length (105583): 2048
 4 name ( [- to delete, * to quit]): swap
 4 start (102071):
 4 length (103490): 20000
 5 name ( [- to delete, * to quit]): fs
 5 start (122071):
 5 length (83490): 
 6 name ( [- to delete, * to quit]): *

Nr Name                 Overlap       Start      End        %     Size
 0 /hd0disk             012345            0   205561      100   205561
 1 /hd0partition        01----       205560   205561        0        1
 2 /hd0dos              0-2---            0   100023       46   100023
 3 /hd0boot             0--3--       100023   102071        1     2048
 4 /hd0swap             0---4-       102071   122071       10    20000
 5 /hd0fs               0----5       122071   205560       43    83490

Ok:y
%
.P2
.IP 28.
Copy the DOS boot program to the DOS partition on the hard disk.
.P1
% cp /b.com /n/c:/b.com
.P2
.IP 29.
Copy a kernel to the boot partition.
This will initially be the floppy's kernel but later should
be a kernel you build specifically for your system.
.P1
% mount -b '#w' /dev
% cp /9dos /dev/hd0boot
.P2
.IP 30.
Now start up a local Plan 9 file server and instruct it to format
the new file system partition.
.P1
% disk/kfs -rf /dev/hd0fs
.P2
.PP
You now have a Plan 9 partition on the disk containing a minimal
user table,
.CW /adm/users.
If you like, you may copy all of the floppy disk onto it (see
.I mkfs (8))
or, if it works, you may get things over the Ethernet using the
.I ftpfs (4)
program.
To do the latter you will have to start up the connection
server, configure the internet, and use the FTP file system
to attach to another system.
The boot floppy's kernel supports both 8 bit and 16 bit
Elite Ethernet boards.
.IP 30.
One thing that the system doesn't work out automatically
is what kind of mouse you have.  Before you can run
.CW 8½ ,
you need to configure the mouse by running one of
.P1
echo ps2 > '#v/mouseconf'
echo serial0 > '#v/mouseconf'
echo serial1 > '#v/mouseconf'
.P2
depending on whether you have a mouse attached to the
PS/2 port, serial port 0 (COMA), or serial port 1 (COMB).
.IP 31.
Getting
.CW ftpfs
started (assuming you are 192.11.4.195 and the
destination is 11.222.333.444).
.P1
% ndb/cs
% ip/ipconfig 192.11.4.195
% ftpfs -/ 11.222.333.444
.P2
.PP
After typing a user name and password that
.CW ftpfs
prompts for,
you should have the root of the other system's file system
mounted under
.CW /n/ftp .
.PP
If you want to avoid constantly typing numbers at ipconfig and
ftpfs, edit the file
.CW /lib/ndb/local.
The format should be obvious from the templates already in the file.
