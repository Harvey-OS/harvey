.TL
Installing the Plan 9 Distribution
.PP
You should read this document through once before unpacking the CD-ROM.
You should also read the
.I intro
pages of the Programmer's Manual \(em the first page in each numbered section \(em
and also
.I namespace (4)
(that is, the page in section 4 titled NAMESPACE),
.I auth (8),
.I boot (8),
.I booting (8),
.I fs (8),
and
.I fsconfig (8)
to gain some familiarity
with the system.
.PP
The disk contains a single ISO-9660 CD-ROM file system.
That file system is a complete Plan 9 distribution, structured
as it will be after you unpack.  The job of unpacking the disk
is therefore just to duplicate the CD-ROM structure on
a regular file system.
You may read the file system with any 9660-supporting software.
However, since Plan 9 file names do not respect 9660
rules and because 9660 directories do not contain quite enough
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
you see all the files as we want you to see them.
You will therefore use Plan 9 to unpack the distribution.
.PP
To install the distribution
you will need two machines connected to a (possibly subnetted)
Ethernet:
.IP \f(CWT\fP:
A terminal with at least 8 megabytes of memory,
either a MIPS Magnum 3000
or some form of Sparcstation.
.IP \f(CWF\fP:
A file server with at least 16 megabytes of memory and
an empty SCSI disk with at least 600 megabytes,
either a MIPS Magnum 3000
or Sparcstation 2.
It need not be the same type of machine as
.CW T .
.PP
By terminal we mean a possibly diskless machine with a frame buffer
and keyboard.
By file server we mean a machine with disks and a terminal acting
as a console.
The file server should
have a 9600 baud
terminal connected to port
.CW ttya
on a Sun and
.CW tty1
on a Magnum; the file server code does not use the frame buffer.
We have tested both the terminal and file
server systems with MIPS Magnum 3000's.
.PP
We believe the terminal system will work with any Sparcstation
1- or 2-class
SUN, but we have only tried it on a Sparcstation-2 and an SLC.
We know it works with
.CW bwtwo
and
.CW cgsix
frame buffers in SBUS slot 3.  We believe it will also work with a
.CW cgthree
but we haven't tried it.
The system uses the frame buffer in the lowest-numbered SBUS slot,
which is what the ROM does, too.
.PP
The Sparc file server code depends on the properties of the DMA
chips in the machine, which vary considerably.  We know it works
on the Sparcstation 2; it is likely to fail on any other model,
but we don't know.
.PP
If you're using a SUN for a terminal, you need to set the environment
variables
.CW input-device
and
.CW output-device
to
.CW keyboard
and
.CW screen :
the ROM needs to initialize the frame buffer for us.
On Magnums, set
.CW bootmode
to
.CW e
on both the terminal and file server.
Set
.CW console
to
.CW m
on the terminal and to
.CW 1
on the file server.
The Magnum may have either the one-bit deep frame buffer or
the 8-bit deep color frame buffer.
.PP
You will also need a SCSI-resident CD-ROM drive connected to the terminal,
.CW T .
If
.CW T
is a SUN, the drive must be on SCSI unit 6, which is SUN's default.
It can have any unit you want on the Magnum; we assume 6 below
but the necessary changes will be obvious.
.PP
Use a Unix system to extract the necessary boot images from the CD-ROM.
For SUNs, you'll need
.CW /sparc/9sscd
for the terminal and
.CW /sparc/9ssfs
for the file server.
For Magnums, you'll need
.CW /mips/9magnumcd
for the terminal and
.CW /mips/9magnumfs
for the file server.
Put these files where needed so they may be bootstrapped from
.CW tftpd
on the appropriate machines.
Later you'll need
.CW /mips/9magnum
and
.CW /sparc/9ss ;
you might as well grab them now, too.
.PP
Now, after the CD-ROM drive is connected and
the distribution disk is in the drive,
boot the terminal
.CW T
from
.CW 9magnumcd
or
.CW 9sscd
as appropriate.
On the SUN, just boot the system without arguments
(type
.P1
boot net
.P2
to the ROM).  On the Magnum you must pass an argument giving the name
of the CD-ROM.  It's easiest to install the system where
.CW BOOTP
can find it and type
.P1
bootp()9magnumcd local!#R6/cd6
.P2
Change the 6's if your CD-ROM is on a different unit.
When the Plan 9 kernel boots it will print the hardware Ethernet address:
.P1
lance ether: 001122334455
.P2
Write this number down.
The system will ask your user name;
type
.CW tor .
For the
.CW password: ,
just type a return.
.PP
At this point the system will mount the CD-ROM file system and start up.
It takes a couple of minutes to get anywhere useful: the CD-ROM
drives we've seen only read about 20K or 30K bytes per second average.
The system will print an error from
.CW ipconfig ;
ignore it.
Eventually the system will run
.CW /usr/tor/lib/profile ,
which starts the window system,
.CW 8½ .
In Plan 9 you can always connect as user
.CW none ,
a deliberately unauthenticated user, but
.CW none
has minimal privileges and no permanent files.
.CW Tor
is a stub account we put on the distribution to show you how a proper user's
directory should be set up.
In particular,
.CW tor
has a
.CW profile
and therefore establishes a workable environment when he logs in.
.PP
You want to use the window system because processes cannot
be interrupted by typing
DEL on the console; in Plan 9 only the window system has
what corresponds to a teletype driver in Unix.
.CW Tor 's
profile runs a script to make an initial window automatically.
You need to click with the left button in the window to make it current,
that is, so you can type in it.
Erase is backspace and line kill is control-U.
These cannot be changed.
(To type the character
.CW ½ ,
type the sequence
.CW Option
.CW 1
.CW 2
on the Magnum,
.CW Compose
.CW 1
.CW 2
on the SUN,
or see
.I keyboard (6)).
To make another window, if you want (there's no need now),
press the right button of the mouse to pop up a menu,
select the
.CW New
menu entry and follow your nose or see the manual.
.PP
In a window, type
.P1
ip/ipconfig 111.112.113.114
.P2
using the correct IP address for the machine
.CW T .
This readies the machine to talk on the Ethernet.
This should be all you need if you and
.CW F
are on the same Ethernet strand.
If you need to use a subnet mask to talk to
.CW F ,
instead type
.P1
ip/ipconfig -m 255.255.255.0 111.112.113.114
.P2
(with the right numbers).  If you need to go through
a router to
.CW F ,
also type
.P1
echo -n 'add 0.0.0.0 0.0.0.0 111.112.113.1' > '#P/iproute'
.P2
where the IP number is that of the router.
(Normally all this is done automatically,
but you don't have a network database yet.)
.PP
Now you need to bootstrap and configure the file server.
.bp
.PP
.I
Carefully read the next page or so before booting the file server.
Some of the actions are dangerous.
.PP
Boot
.CW F
from the file
.CW 9magnumfs
or
.CW 9ssfs
just as you would boot the terminal.  No arguments are needed.
The system will chatter a bit and eventually print
.P1
 ** NVR config checksum is incorrect **
.P2
and enter `config' mode.  If you have weird luck and the checksum
in non-volatile RAM is correct, the system will say
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
Then set the machine's IP address, its IP mask, the IP address of
its gateway by typing (using the correct addresses):
.P1
ip 123.111.112.113
ipgw 123.111.112.1
ipmask 255.255.255.0
.P2
Although you don't have an authentication server yet,
if you did you would also need to set
.CW ipauth .
.PP
Now you must establish the disk configuration.  First pick a single
SCSI disk
that will be the repository of the configuration information.
This information is held in a block (number zero) not otherwise used by the
file system.  Typically, but not necessarily,
it will live on the same drive as the main file system.
Say that's SCSI unit 4.  Then type
.P1
config w4
.P2
The
.CW w
stands for `wren', our generic name for SCSI disks.
Next you must define the layout of the disks that the machine will
use.
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
All the configuration commands do is set up data structures; they
do no real work.  Once you leave config mode, the file system will
establish the structure you described.  This means that if you make
a mistake you can reset the machine without doing any damage.
To get out of config mode and on to real work, type
.P1
end
.P2
(Note: on the Sparcs, writing the configuration information
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
The same problem applies to writing the password; see below.)
.PP
After you leave config mode, the file server
will initialize its disk, complain a little, and prompt you:
.P1
kremvax:
.P2
The prompt may get lost among the other output; type a couple of newlines.
Somewhere, on a line beginning
.CW lance
.CW input ,
it will have printed its Ethernet address; make a note of it.
.PP
If you need to reboot
.CW F
before you have unpacked correctly, you will need to
ream the disk again (from config mode) and start over.
Reaming the disk turns off permission checking to enable the
unpacking process to set file owners and modes correctly,
and there is no other way to set that mode.
.PP
Now type on
.CW F
the command
.P1
users default
.P2
to establish a minimal set of users to own the files unpacked from the disk.
See
.I fs (8)
for more information about these commands.
.PP
At this point the file system is prepared to be sent some data.
Check that you can connect to it:
if
.CW F's
IP address is 111.123.123.99,
in a window back on
.CW T
type
.P1
srv il!111.123.123.99
.P2
This command should print
.P1
nop...session...post...
.P2
and give you a new prompt, signalling that it has dialed
.CW F
successfully.
If it doesn't, make sure the machines are up and the network is happy.
.PP
If all is well, you're ready to unpack.
.PP
.PP
Type the following commands.
They should print nothing and, except for the last,
complete almost instantaneously.
.P1
mount -c /srv/il!* /n/kremvax
mount /srv/boot /n/boot
disk/mkfs -a -s /n/boot /lib/proto/allproto |
 time disk/mkext -u -d /n/kremvax ; chmod 770 /n/kremvax/sys/src
.P2
The first command mounts the root of
.CW F 's
tree on
.CW /n/kremvax ,
a directory we included in the distribution.
The second mounts the root of the CD-ROM afresh on
.CW /n/boot ,
so that any local modifications to the name space (such as mounts
on
.CW /dev )
do not confuse the unpacking.
Then, the third command (which takes hours)
packs up the CD-ROM hierarchy as a sort
of archive and unpacks it again using the 9P file system protocol
over the Ethernet to
.CW F .
The
.CW chmod
is to protect the source from the prying eyes of people not in group
.CW sys ,
as explained below.
.CW Tor
is not in group
.CW sys ,
so you must do the
.CW chmod
.I after
the unpack; otherwise you would be unable to unpack the source tree.
.PP
You should see no error messages.  If you do, a likely mistake is
that you forgot to type
.P1
ream main
.P2
or
.P1
users default
.P2
on
.CW F
so
.CW F
is not primed to receive the distribution.
The unpacking will take from 4 to 6 hours because CD-ROM devices
are so slow.
While it runs, get a cup of coffee, eat dinner, read the Plan 9
manual, or do whatever local pastime seems natural.
.PP
When the final command completes, type
.P1
cat /notice
.P2
and memorize the output to meet our legal obligations, then type
.P1
du /n/kremvax/usr/tor
.P2
You should see
.P1
0	/n/kremvax/usr/tor/bin/386
0	/n/kremvax/usr/tor/bin/68020
0	/n/kremvax/usr/tor/bin/hobbit
0	/n/kremvax/usr/tor/bin/mips
1	/n/kremvax/usr/tor/bin/rc
0	/n/kremvax/usr/tor/bin/sparc
1	/n/kremvax/usr/tor/bin
1	/n/kremvax/usr/tor/lib
0	/n/kremvax/usr/tor/tmp
2	/n/kremvax/usr/tor
.P2
That's the last thing to get unpacked, so this confirms that you got
everything.
.PP
Now go back to
.CW F 's
console and type
.P1
check
.P2
After about a minute the command should print something like
.P1
checking filsys: main
lo = 69408; hi = 108319
nfiles = 11790
fsize  = 334605
nused  = 94933
ndup   = 0
nfree  = 239670
tfree  = 239670
nfdup  = 0
nmiss  = 0
nbad   = 0
nqbad  = 0
maxq   = 11789
.P2
where the
.CW lo ,
.CW hi ,
and free numbers will depend on the size of your disk.
The other numbers you get
may be somewhat different from these values; as this document
is being written the exact numbers aren't known.
.PP
While the unpacking is going on, you may type
.CW stats
.CW -al
on
.CW F 's
console
to see how the I/O throughput is going.
You may also type
.CW check
during the unpack, but do so carefully as it freezes file I/O while it runs.
These commands are documented in
.I fs (8).
.PP
The directory
.CW /tmp
is now a deliberately unwritable directory, even on
.CW F
(see
.I namespace (6)).
Type
.P1
ramfs
.P2
to drop a RAM-resident temporary file system on
.CW /tmp
so you can run
.CW ed
or, if you're familiar with it,
.CW sam .
.PP
The next step is to establish a network database; see
.I ndb (6).
First change directory to
.CW /n/kremvax/lib/ndb :
.P1
cd /n/kremvax/lib/ndb
.P2
Use the editor
.CW ed "" (
or
.CW sam )
to
add to the file
.CW local
a description of the two machines you are using.
The format should be self-explanatory; it's also covered in
.I ndb (6).
The example for
.CW kremvax
in the file as distributed should serve as a guide:
.P1
sys = kremvax
	dom=kremvax.kremlin.mos.su
	bootf=/mips/9magnum
	ip=1.1.1.1 ether=0123456789AB
	proto=il
.P2
Make sure the line
.CW proto=il
is there; this causes its clients to use
the fast Plan 9 Internet datagram protocol, IL.
That's the protocol you used to unpack the disk.
It's assumed that any machine with IP support can talk TCP,
which will mean that you can talk to Unix machines using TCP.
.PP
Set the domain
.CW dom ) (
to that of your site.
.PP
The start of the
.CW local
file should now look something like this:
.P1
sys = T
        dom=T.research.att.com
	bootf=/mips/9ss
        ip=101.101.101.102 ether=010203040506
        proto=il

sys = F
        dom=F.research.att.com
	bootf=/mips/9ssfs
        ip=101.101.101.103 ether=001122334455
        proto=il
.P2
You need the Ethernet addresses
to help the machines discover their own names.
Leave the tail of the file alone.
.PP
Next, use
.CW ftpfs
to grab a copy of
.CW /etc/hosts
from a local Unix machine:
.P1
ftpfs -/ 112.123.114.92
.P2
(Use the Unix machine's IP address, of course.)
The Unix machine will ask you to log in for FTP.
Do so normally.
.CW Ftpfs
will return, after which the directory
.CW /n/ftp
is connected to the remote Unix machine, so
.P1
cp /n/ftp/etc/hosts .
.P2
Now
correct the domain in the file
.CW /n/kremvax/lib/ndb/mkfile
.CW grep "" (
for
.CW kremlin
in
.CW mkfile
to see where to make the changes ).
In
.CW mkfile
the domain is used to select between local and global addresses
when building the network data base.
.PP
Next, on machine
.CW T ,
in directory
.CW /n/kremvax/lib/ndb ,
type
.P1
mk
.P2
This will run for a minute or two (depending on the size of your
.CW hosts
file) and build a hash index of the database.
.PP
Check that the database is right.  Type
.P1
bind -c /n/kremvax/lib/ndb /lib/ndb
ndb/query sys F
.P2
It should immediately print something like
.P1
sys=F dom=kremvax.kremlin.mos.su ip=101.101.101.103 proto=il
.P2
If not, you've made a mistake in setting up the database.
.PP
At this point you should reboot
.CW T ,
leaving
.CW F
up.
To shut down, either hit the reset button
(no
.CW sync
is necessary or even possible)
or type two control-T's followed by an
.CW r .
.PP
Now reboot
.CW T
using BOOTP to load the regular operating system \(em
.CW 9ss
or
.CW 9magnum ,
not the CD-ROM one \(em
and tell it to use an IL connection to
.CW F
as its root, as follows.
On a Magnum,
.P1
bootp()9magnum il!IP.address.of.F
.P2
On a Sparc, copy
.CW /sparc/9ss
to the appropriate place on the Unix system and type
.P1
boot net
.P2
to the ROM.  The Sparc will try to find a file server, fail (because
only Plan 9's own BOOTP has that information), and ask you to fill
in the missing information:
.P1
My IP address[0.0.0.0]:
.P2
Supply
.CW T 's
IP address in the form
.P1
101.102.101.103
.P2
Next it will ask for
.P1
My IP mask[255.255.255.0]:
.P2
This is the subnet mask for
.CW T :
supply the right answer.  Next it will ask for
.P1
My IP gateway[0.0.0.0]:
.P2
This is the IP address of the gateway
.CW T
is on; supply that.
.PP
Finally you will be asked again for your user name; continue to
pretend you're
.CW tor .
.PP
The system should come up and give you a window system.
.PP
You're up!
.PP
Now you need to set up some accounts and start authentication (see
.I auth (8)).
You want to get this right before rebooting
.CW F ;
remember that
.CW F
is still in the initialization state.
.PP
Check that directory
.CW /adm
is mode
.CW rwxrwxrwx
and the other files in that directory are
.CW rw-rw-rw-.
The directory
.CW /adm/timezone
should be
.CW rwxrwxr-x .
These permissions will make it easy to do the following steps.
If you make a horrible mistake, you will be able to back out
by just editing the files as any user.
Once you trust that things are right (in a day or two), you should remove write
permission for most people, perhaps for everyone but the owner of the file.
Once authentication is enabled, if you make a horrible mistake you will have
to remove
.CW /adm/users
and
.CW /adm/keys
and recreate them as writable files, all from the console of
.CW F .
.PP
Here is how we recommend you set up permissions in
.CW /adm :
.P1
--rw-rw-r-- M 7 adm adm       0 Aug 23 21:28 /adm/cache
--rw-rw-r-- M 7 adm adm    8774 Aug 23 21:28 /adm/keys
--rw-rw-r-- M 7 adm adm       0 Aug 23 21:28 /adm/kprofdata
--rw-rw-r-- M 7 adm adm    7872 Aug 22 22:48 /adm/netkeys
d-rwxrwxr-x M 7 adm sys       0 Aug  7 16:33 /adm/timezone
--rw-r--r-- M 7 adm adm    3812 Aug 19 09:43 /adm/users
a-rw-rw-rw- M 7 adm adm       0 Aug 19 09:43 /adm/whois
.P2
Note that
.CW /adm/users
is unwritable by anyone but
.CW adm
so
.CW /adm/users
can be changed only using the
.CW newuser
command on the file server console.
The two keys files are writable by group
.CW adm
and the machine logins (see below) are in that group.
While
.CW F
is in the initialization state, you can use the
.CW chgrp
command to make these changes or, again, you can
remove and recreate the files from
.CW F 's
console.
.PP
Make a list of the users you want to add now.
This should include a few real people and at least one
machine login.
The machine login is a name compiled into the operating systems;
as shipped the name is
.CW bootes
but you can (and perhaps should) change it after everything is working.
We run all our machines with the same machine login, but it would be
fine to give each machine its own login.
Each machine login must have the same password, however, as each
machine may need to decrypt the keys file, and the machine's password
is used for the decryption.
.PP
Edit the file
.CW /adm/users
to add the new users.  The format of the file is easy; as shipped
it looks like this (see
.I users (6)):
.P1
-1:adm:adm:bootes
0:none:adm:
1:tor:tor:
10000:sys::
10001:map:map:
10002:doc::
10003:upas:upas:
10004:cda::
10005:bootes:bootes:
.P2
The users after the last colon on each line are the
.I members
of the group; unlike in Unix, groups and users come from the same file,
and a group is just a user with members.
By a convention you should honor, groups are numbered starting at 10000.
You should leave the
.CW bootes
line alone for now and add each machine login to the
.CW adm
group.
We put our programmers in group
.CW sys
and have our source and binaries owned by that group.
.PP
After you've made the edits and written back the file, type
.P1
users
.P2
on
.CW F 's
console.
This should claim a sensible number of users are known \(em exactly
the number of lines in
.CW /adm/users .
.CW F
is now running with permissions as defined in this file.
.PP
If you made a mistake and the system complains about the file, type
.P1
users default
.P2
again and fix the file.
.PP
If you want to add a new user after authentication is started, use the
.CW newuser
command on the file system console, which creates the user directories
automatically.
We're doing it the other way because
it's easier to get the
.CW users
file right from a text editor when you're adding a lot of users.
.PP
Now set the key to be used to decrypt the keys file. On
.CW T
run
.P1
auth/wrkey
.P2
and remember this password.  You will need to set all the machine
logins to have this password.
.PP
Now start the key file system:
.P1
auth/keyfs
.P2
It should print
.P1
0 keys read
.P2
.PP
Now for each user who should be able to log in,
set a password.
To do this, type
.P1
auth/adduser bootes
.P2
to set the password for
.CW bootes ,
for example.
The program will prompt for a password.
Again, make sure all the machine logins have the same password.
The groups
.CW adm
and
.CW sys
and the user
.CW none
should not have passwords.
.PP
Kill and restart the key file system:
.P1
kill keyfs | rc
auth/keyfs
.P2
This time it should claim that it has some keys, one for each user you
installed.
.PP
Now move to
.CW F 's
console and get its authentication going.  Type
.P1
users
.P2
again if you've made any changes.
Now set the machine password to be exactly the same as for the machine
logins (the file server never actually logs in, so it doesn't have
a user name internally, but it does need to decrypt the keys file):
.P1
passwd
.P2
Finally run
.P1
auth
.P2
to start authentication running from the keys in
.CW /adm/keys .
You can set up authentication to be done by a separate server, but
now is not the time for that.
.PP
Now for each new user you've added, reboot
.CW T
and make sure you can log in as that user.
If you have trouble, you will still be able to log in as
.CW none
with no password.  You will
.I always
be able to log in as
.CW none
with no password but with restricted permissions.
.PP
Once you're confident things are set up correctly, shut down
.CW T .
Then shut down
.CW F
by typing
.P1
halt
.P2
on its console.
Now boot
.CW F
as above, but don't ream the disk.
You will have to type the machine password to be able
to type commands.
.PP
For each new user, reboot
.CW T ,
log in as the user, and type
.P1
rc /sys/lib/newuser
.P2
to initialize the user's directory (see
.I newuser (8)).
You can look at the directory
.CW /usr/tor
to see how things should look.
.PP
Sometime soon, you should check that you can recompile the operating
systems, change the login used by machines, the variable
.CW service
in the file server, and things like that.
The file
.CW /adm/timezone/timezone
contains the default timezone; you may want to change this.
The file
.CW /lib/sky/here
contains the local geographic coordinates, which are certainly
not the same as at Murray Hill.
.PP
Finally, log in as a real user and type:
.P1
swap /tmp
scat
ori
plot nogrid
.P2
(the third and fourth lines are input to the command on the second line).
You should, after a pause, see a map of Orion.
.PP
When you get here, the system is up, you can compile, edit, and
so on.
.PP
Read the manual.
Make sure to look at
.I u9fs (4)
to see how to set up Plan 9 access to Unix file systems.
Because Plan 9 has a unique authentication structure,
the reverse direction can only be done using
.CW ftp .
See the directory
.CW /sys/src/cmd/unix
for some Unix-resident programs whose source we've included.
.PP
You have a system!
