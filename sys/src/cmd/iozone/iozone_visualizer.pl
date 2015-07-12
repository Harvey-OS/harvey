#!/usr/bin/env perl

use warnings;
use strict;

# arguments: one of more report files
#
# Christian Mautner <christian * mautner . ca>, 2005-10-31
# Marc Schoechlin <ms * 256bit.org>, 2007-12-02
# Pär Karlsson <feinorgh * gmail . com>, 2012-07-24
#
# This script is just a hack :-)
#
# This script is based loosely on the Generate_Graph set
# of scripts that come with iozone, but is a complete re-write
#
# The main reason to write this was the need to compare the behaviour of
# two or more different setups, for tuning filesystems or
# comparing different pieces of hardware.
#
# This has been updated (2012) to take the new gnuplot 4 syntax into account,
# and some general perl cleanups, despite its "hack" status.
# It also requires some perl libraries to funktion, specifically
# Readonly and List::MoreUtils which are not included in the standard
# perl in most distributions.
#
# This script is in the public domain, too short and too trivial
# to deserve a copyright.
#
# Simply run iozone like, for example, ./iozone -a -g 4G > config1.out (if your machine has 4GB)
#
# and then run perl report.pl config1.out
# or get another report from another box into config2.out and run
# perl report.pl config1.out config2.out
# the look in the report_* directory for .png
#
# If you don't like png or the graphic size, search for "set terminal" in this file and put whatever gnuplot
# terminal you want. Note I've also noticed that gnuplot switched the set terminal png syntax
# a while back, you might need "set terminal png small size 900,700"

use Carp;
use English qw(-no_match_vars);
use Getopt::Long;
use List::MoreUtils qw(any);
use Readonly;

Readonly my $EMPTY => q{};
Readonly my $SPACE => q{ };

my $column;
my %columns;
my $datafile;
my @datafiles;
my $outdir;
my $report;
my $nooffset = 0;
my @reports;
my @split;
my $size3d;
my $size2d;

# evaluate options
GetOptions(
    '3d=s'     => \$size3d,
    '2d=s'     => \$size2d,
    'nooffset' => \$nooffset
);

if ( not defined $size3d ) {
    $size3d = '1280,960';
}
if ( not defined $size2d ) {
    $size2d = '1024,768';
}

my $xoffset = 'offset -7';
my $yoffset = 'offset -3';

if ( $nooffset == 1 ) {
    $xoffset = $EMPTY;
    $yoffset = $EMPTY;
}

print <<"_TEXT_";
iozone_visualizer.pl : this script is distributed as public domain
Christian Mautner <christian * mautner . ca>, 2005-10-31
Marc Schoechlin <ms * 256bit.org>, 2007-12-02
Pär Karlsson <feinorgh * gmail . com>, 2012-07-24
_TEXT_

@reports = @ARGV;

if ( not @reports or any { m/^-/msx } @reports ) {
    print "usage: $PROGRAM_NAME --3d=x,y -2d=x,y <iozone.out> [<iozone2.out>...]";
    exit;
}

if ( any { m{/}msx } @reports ) {
    croak 'report files must be in current directory';
}

print
"Configured xtics-offset '$xoffset', configured ytics-offset '$yoffset' (disable with --nooffset)\n";
print 'Size 3d graphs : ' . $size3d . " (modify with '--3d=x,y')\n";
print 'Size 2d graphs : ' . $size2d . " (modify with '--2d=x,y')\n";

#KB reclen write rewrite read reread read write read rewrite read fwrite frewrite fread freread
%columns = (
    'KB'         => 1,
    'reclen'     => 2,
    'write'      => 3,
    'rewrite'    => 4,
    'read'       => 5,
    'reread'     => 6,
    'randread'   => 7,
    'randwrite'  => 8,
    'bkwdread'   => 9,
    'recrewrite' => 10,
    'strideread' => 11,
    'fwrite'     => 12,
    'frewrite'   => 13,
    'fread'      => 14,
    'freread'    => 15,
);

#
# create output directory. the name is the concatenation
# of all report file names (minus the file extension, plus
# prefix report_)
#
$outdir = 'report_' . join q{_}, map { m/([^.]+)([.].*)?/msx && $1 } (@reports);

print {*STDERR} "Output directory: $outdir ";

if ( -d $outdir ) {
    print {*STDERR} '(removing old directory) ';
    system "rm -rf $outdir";
}

mkdir $outdir or croak "cannot make directory $outdir";

print {*STDERR} "done.\nPreparing data files...";

for my $report (@reports) {
    open my $fhr, '<', $report or croak "cannot open $report for reading";
    $report =~ m/^([^\.]+)/msx;
    $datafile = "$1.dat";
    push @datafiles, $datafile;
    open my $fho3, '>', "$outdir/$datafile"
      or croak "cannot open $outdir/$datafile for writing";
    open my $fho2, '>', "$outdir/2d-$datafile"
      or croak "cannot open $outdir/$datafile for writing";

    my @sorted = sort { $columns{$a} <=> $columns{$b} } keys %columns;
    print {$fho3} '# ' . join ($SPACE, @sorted) . "\n";
    print {$fho2} '# ' . join ($SPACE, @sorted) . "\n";

    my $old_x = 0;
    while (<$fhr>) {
        my $line = $_;
        if ( not $line =~ m/^[\s\d]+$/msx ) {
            next;
        }
        @split = split();
        if ( @split != 15 ) {
            next;
        }
        if ( $old_x != $split[0] ) {
            print {$fho3} "\n";
        }
        $old_x = $split[0];
        print {$fho3} $line;
        if ( $split[1] == 16384 or $split[0] == $split[1] ) {
            print {$fho2} $line;
        }
    }
    close $fhr  or carp $ERRNO;
    close $fho3 or carp $ERRNO;
    close $fho2 or carp $ERRNO;
}

print {*STDERR} "done.\nGenerating graphs:";

open my $fhtml, '>', "$outdir/index.html"
  or carp "cannot open $outdir/index.html for writing";

print {$fhtml} <<'_HTML_';
<?xml version="1.0" encoding="iso-8859-1"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">
<head>
<title>IOZone Statistics</title>
 <STYLE type="text/css">
.headline { font-family: Arial, Helvetica, sans-serif; font-size: 18px; color: 003300 ; font-weight: bold; text-decoration: none}
 </STYLE>
</head>
<body>
<a name="top"></a>
<h1>IOZone Statistics</h1>
<table width="100%" summary="iozone stats">
<tr>
<td>
_HTML_

# Generate Menu
print {$fhtml} <<'_HTML_';
<u><b>## Overview</b></u>
<ul>
_HTML_
for my $column ( keys %columns ) {
    if ( $column =~ m/KB|reclen/msx ) {
        next;
    }
    print {$fhtml} '<li><b>'
      . uc($column)
      . '</b> : '
      . '<a href="#'
      . $column
      . "\">3d</a>\n"
      . '<a href="#s2d-'
      . $column
      . "\">2d</a></li>\n";
}
print {$fhtml} "</ul></td></tr>\n";

# Generate 3D plots
for my $column ( keys %columns ) {
    if ( $column =~ m/KB|reclen/msx ) {
        next;
    }
    print {*STDERR} " $column";

    open my $fhg, '>', "$outdir/$column.do"
      or croak "cannot open $outdir/$column.do for writing";

    print {$fhg} <<"_GNUPLOT_";
set title "Iozone performance: $column"
set grid lt 2 lw 1
set surface
set xtics $xoffset
set ytics $yoffset
set logscale x 2
set logscale y 2
set autoscale z
set xlabel "File size in Kbytes"
set ylabel "Record size in Kbytes"
set zlabel "Kb/s"
set style data lines
set dgrid3d 80,80,3
set terminal png small size $size3d nocrop
set output "$column.png"
_GNUPLOT_

    # original term above:
    print {$fhtml} <<"_HTML_";
      <tr>
       <td align="center">
         <h2><a name="$column"></a>3d-$column</h2><a href="#top">[top]</a><BR/>
         <img src="$column.png" alt="3d-$column"/><BR/>
       </td>
      </tr>
_HTML_

    print {$fhg} 'splot ' . join q{, },
      map { qq{"$_" using 1:2:$columns{$column} title "$_"} } @datafiles;

    print {$fhg} "\n";

    close $fhg or carp $ERRNO;

    open $fhg, '>', "$outdir/2d-$column.do"
      or croak "cannot open $outdir/$column.do for writing";
    print {$fhg} <<"_GNUPLOT_";
set title "Iozone performance: $column"
#set terminal png small picsize 450 350
set terminal png medium size $size2d nocrop
set logscale x
set xlabel "File size in Kbytes"
set ylabel "Kbytes/sec"
set output "2d-$column.png"
_GNUPLOT_

    print {$fhtml} <<"_HTML_";
      <tr>
       <td align="center">
         <h2><a name="s2d-$column"></a>2d-$column</h2><a href="#top">[top]</a><BR/>
         <img src="2d-$column.png" alt="2d-$column"/><BR/>
       </td>
      </tr>
_HTML_

    print {$fhg} 'plot ' . join q{, },
      map { qq{"2d-$_" using 1:$columns{$column} title "$_" with lines} }
      (@datafiles);

    print {$fhg} "\n";

    close $fhg or carp $ERRNO;

    if ( system "cd $outdir && gnuplot $column.do && gnuplot 2d-$column.do" ) {
        print {*STDERR} '(failed) ';
    }
    else {
        print {*STDERR} '(ok) ';
    }
}

print {$fhtml} <<'_HTML_';
</table>
</body>
</html>
_HTML_
print {*STDERR} "done.\n";

