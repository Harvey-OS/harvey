#!/usr/local/bin/perl
#
#   MakeTeXPK.pl
#
#   v1.0 - modified by John Stoffel (john@wpi.wpi.edu) from the original
#          shell script written by Tomas Rokicki (rokicki@cs.stanford.edu).
#          please feel free to make any modifications you would like to this
#          script, but please acknowledge myself and tom when you make 
#          changes.
#
#        - This was orignally modified to write the fonts to a seperate 
#          directory because the fonts were stored on a read-only NFS
#          server.  New fonts were then stored in a second location that
#          was world writeable, so fonts could be created automatically.  
#          
#          1. checks both directories before hand for the font's existence.
#          2. creates the font, then moves it to the writeable directory.
#          3. changes the ownership and protection so users can't write
#             the font directly.
#
#        - this script should NOT be used directly, but run through
#          suidscript.pl first and then used as directed.
#
#   todo:
#        - add automagic support for write-white and write-black printers.
#          now I have two seperate version of the same program.  This would
#          mean adding in either a new parameter, or possibly a -w or -b
#          switch.  Default could be customizable.  What do you think tom?  
#      
#        - cleanup the code a little more and write it in better perl.
#
# ------------------------------------------------------------------------
#
#   This script file makes a new TeX PK font, because one wasn't
#   found.  Parameters are:
#
#   name dpi bdpi magnification [[mode] subdir]
#
#   `name' is the name of the font, such as `cmr10'.  `dpi' is
#   the resolution the font is needed at.  `bdpi' is the base
#   resolution, useful for figuring out the mode to make the font
#   in.  `magnification' is a string to pass to MF as the
#   magnification.  `mode', if supplied, is the mode to use.
#
#   Note that this file must execute Metafont, and then gftopk,
#   and place the result in the correct location for the PostScript
#   driver to find it subsequently.  If this doesn't work, it will
#   be evident because MF will be invoked over and over again.
#
#   Of course, it needs to be set up for your site.
#
# -------------------------------------------------------------------------

# setup the environment variables before hand.

$ENV{'PATH'} = '/bin:/usr/bin:/usr/ucb:/usr/local/bin';
$ENV{'SHELL'} = '/bin/sh' if $ENV{'SHELL'} ne '';
$ENV{'IFS'} = '' if $ENV{'IFS'} ne '';
$path = $ENV{'PATH'};
umask(0022);

# set who the owner and group of the created fonts will be.

$OWNER = "root.tex";

# check number of arguements.

die "Not enough arguments!\n" if ($#ARGV < 3);

# make sure the user doesn't try to give us any control characters as
# as arguements.

$NAME=&untaint($ARGV[0]);
$DPI=&untaint($ARGV[1]);
$BDPI=&untaint($ARGV[2]);
$MAG=&untaint($ARGV[3]);
$MODE=&untaint($ARGV[4]) if (defined($ARGV[4]));
$PK=&untaint($ARGV[5]) if (defined($ARGV[5]));

# texdir and local dir can be the same if $TEXDIR is world writeable, or 
# different if $TEXDIR is read-only and $LOCALDIR is read-write.

$TEXDIR="/usr/local/lib/tex";
$LOCALDIR="/shared/tex/fonts";
$DESTDIR="$LOCALDIR/white/pk";

# TEMPDIR needs to be unique for each process because of the possibility
# of simultaneous processes running this script.

if ($TMPDIR eq '') {
    $TEMPDIR="/tmp/mtpk.$$";
   }
else {
    $TEMPDIR="$TMPDIR/mtpk.$$";
   }

if ($MODE eq "") {
    if ($BDPI eq "300") { $MODE='imagen'; }
    elsif ($BDPI eq "200") { $MODE='FAX'; }
    elsif ($BDPI eq "360") { $MODE='nextII'; }
    elsif ($BDPI eq "400") { $MODE='nexthi'; } 
    elsif ($BDPI eq "100") { $MODE='nextscreen'; }
    elsif ($BDPI eq "635") { $MODE='linolo'; }
    elsif ($BDPI eq "1270") { $MODE='linohi'; }
    elsif ($BDPI eq "2540") { $MODE='linosuper'; }
    else {
      die "I don't know the $MODE for $BDPI\nHave your system admin update MakeTeXPK.pl\n"
      }
}

#  Something like the following is useful at some sites.
# DESTDIR=/usr/local/lib/tex/fonts/pk.$MODE

$GFNAME="$NAME.$DPI"."gf";
$PKNAME="$NAME.$DPI"."pk";

# Clean up on normal or abnormal exit

chdir("/") || die "Couldn't cd to /: $!\n";

if (-d $TEMPDIR) {
    rmdir($TEMPDIR) || die "Couldn't remove $TEMPDIR: $!\n";
}
if (-e "$DESTDIR/pktmp.$$") {
    unlink("$DESTDIR/pktmp.$$") || die "Couldn't rm $DESTDIR/pktmp.$$: $!\n";
}

if (! -d $DESTDIR) {
    mkdir($DESTDIR,0755) || die "Couldn't make $DESTDIR: $!\n";
}

if ($PK ne '') {
    $DESTDIR = $DESTDIR . $PK;
    if (! -d $DESTDIR) {
	mkdir($DESTDIR,0755) || die "Couldn't make $DESTDIR: $!\n";
    }
}			       

mkdir($TEMPDIR,0755) || die "Couldn't make $TEMPDIR: $!\n";

chdir($TEMPDIR) || die "Couldn't cd to $TEMPDIR: $!\n";

if (-e "$DESTDIR/$PKNAME") {
    die "$DESTDIR/$PKNAME already exists!\n";
}

# check also in the standard place

if ($PK eq '') {
    if (-e "$TEXDIR/fonts/white/pk/$PKNAME") {
	die "$TEXDIR/fonts/white/pk/$PKNAME already exists!\n";
    }
    elsif (-e "$TEXDIR/fonts/white/pk/$PK$PKNAME") {
	die "$TEXDIR/fonts/white/pk/$PK$PKNAME already exists!\n";
    }
}

# print out the command string we will use, then actually do the command,
# printing it's results.

print "mf \"\\mode:=$MODE; mag:=$MAG; scrollmode; input $NAME\" </dev/null\n";
system("mf \"\\mode:=$MODE; mag:=$MAG; scrollmode; input $NAME\" </dev/null");

# check that $GFNAME was created correctly.

if (! -e $GFNAME ) { die "Metafont failed for some reason on $GFNAME\n";}

print `gftopk -v ./$GFNAME ./$PKNAME`;

# Install the PK file carefully, since others may be doing the same
# as us simultaneously.

`mv $PKNAME $DESTDIR/pktmp.$$`;
chdir($DESTDIR) || die "Couldn't cd to $DESTDIR: $!\n";
`mv pktmp.$$ $PKNAME`;

# now we want to make sure only proper people can change this new font.

`/etc/chown $OWNER $PKNAME`;
`/bin/chmod 664 $PKNAME`;

# this subroutine makes sure there are no funny control characters in 
# the arguements that have been passed to the program.

sub untaint {
    local($temp) = @_;
    $temp =~ /^([-\/\(\)\.\w]*)$/ || die "Invalid arguement: $temp\n";
    $temp = $1;
    return($temp);
}








