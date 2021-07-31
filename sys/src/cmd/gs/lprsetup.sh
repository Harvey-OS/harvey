#!/bin/sh
#
# BSD PRINT FILTER SETUP utility for Ghostscript - used and tested on
# SunOS 4.1.3, but I hope it will be useful on other BSD systems
# See documentation for usage
#

DEVICES="cdj550.24.dq cdj550.16.dq cdj550.8.dq cdj550.3.dq cdj550.1.dq deskjet djet500.dq"
#FILTERS="if nf tf gf vf df cf rf"
FILTERS="if"

GSDIR=/usr/local/lib/ghostscript
GSFILTERDIR=$GSDIR/filt
SPOOLDIR=/var/spool
GSIF=unix-lpr.sh
PCAP=printcap.insert

PATH=/bin:/usr/bin:/usr/ucb
export PATH

if [ ! -w $GSDIR ]; then
  echo "$GSDIR must be writable to create filter directory"
  exit 1
fi

echo "
Making links in the filter directory $GSFILTERDIR ...
"

#
# Make the directory for holding the filter and links
#
if [ -d $GSFILTERDIR ]; then
  echo "$GSFILTERDIR already exists - not created"
else
  mkdir $GSFILTERDIR
fi
rm -f $GSFILTERDIR/direct
ln -s . $GSFILTERDIR/direct
rm -f $GSFILTERDIR/indirect
ln -s . $GSFILTERDIR/indirect

#
# Create a link from each filtertype to the real filter script
#
for filter in $FILTERS
do
  rm -f $GSFILTERDIR/gs$filter
  ln -s  ../$GSIF $GSFILTERDIR/gs$filter
done

#
# Create a link from each device to the filter directory
#
for device in $DEVICES
do
  dualqueue=
  case "$device" in
    *.dq) device=`basename $device .dq` ; dualqueue=t ;;
  esac
  rm -f $GSFILTERDIR/$device
  if [ $dualqueue ]; then
    rm -f $GSFILTERDIR/indirect/$device
    ln -s . $GSFILTERDIR/indirect/$device
  else
    rm -f $GSFILTERDIR/direct/$device
    ln -s . $GSFILTERDIR/direct/$device
  fi
done

#
# Create a basic printcap insert - this is made in the CURRENT directory
#
rm -f $PCAP
cat > $PCAP << EOF
# This is an example printcap insert for Ghostscript printers
# You will probably want either to change the names for each printer
# below (first line for each device) to something more sensible, or
# to add additional name entries (eg cdjcolor for cdj500.24)
# The example is shown set up for serial printers - you will need
# to alter the entries for a parallel or networked remote printer,
# eg. a remote network printer would have a line something like:
#    :lp=:rm=artemis:rp=LPT1:
# for a PC called artemis, replacing the serial port settings
#
# NB/ This is only an example - it is unlikely to be complete or exactly
# correct for your system, but is designed to illustrate filter names 
# corresponding to the accompanying bsd-if print filter
EOF

(
previous=undefined
for device in $DEVICES
do
  dualqueue=
  case "$device" in
    *.dq) device=`basename $device .dq` ; dualqueue=t ;;
  esac
  case "$device" in
    *.24) base=`basename $device .24` ;;
    *.16) base=`basename $device .16` ;;
    *.8)  base=`basename $device .8` ;;
    *.3)  base=`basename $device .3` ;;
    *.1)  base=`basename $device .1` ;;
    *)    base=$device ;;
  esac
#
# If device listed with '.dq' suffix, we set up a separate output queue
#
  if [ $dualqueue ]; then
    if [ $base != $previous ]; then
      previous=$base
      echo "
# Entry for raw device $base.raw
$base.raw|Raw output device $base:\\
    :lp=/dev/ttyb:br#19200:xc#0177777:\\
    :ms=-parity,ixon,-opost:\\
    :sd=$SPOOLDIR/$base/raw:\\
    :mx#0:sf:sh:rs:"
    fi
    echo "
# Entry for device $device (output to $base.raw)
$device|Ghostscript device $device:\\
    :lp=/dev/null:\\"
  else
    echo "
# Entry for device $device
$device|Ghostscript device $device:\\
    :lp=/dev/ttyb:br#19200:xc#0177777:\\
    :ms=-parity,ixon,-opost:\\"
  fi
  echo "\
    :sd=$SPOOLDIR/$base:\\
    :lf=$SPOOLDIR/$base/logfile:\\
    :af=$SPOOLDIR/$base/acct:\\"
  for filter in $FILTERS
  do
    if [ $dualqueue ]; then
      echo "\
    :$filter=$GSFILTERDIR/indirect/$device/gs$filter:\\"
    else
      echo "\
    :$filter=$GSFILTERDIR/direct/$device/gs$filter:\\"
    fi
  done
  echo "\
    :mx#0:sf:sh:rs:"
done
) >> $PCAP

echo "
Example printcap insert file \"$PCAP\" now created"

#
# Remind the user what's still to do
#

echo "
NB/ You will need to create the following directories, with
appropriate permissions, and do 'touch logfile' and 'touch acct'
in the top level directories (ie. not the 'raw' ones):
"
(
for device in $DEVICES
do
  dualqueue=
  case "$device" in
    *.dq) device=`basename $device .dq` ; dualqueue=t ;;
  esac
  case "$device" in
    *.24) base=`basename $device .24` ;;
    *.16) base=`basename $device .16` ;;
    *.8)  base=`basename $device .8` ;;
    *.3)  base=`basename $device .3` ;;
    *.1)  base=`basename $device .1` ;;
    *)    base=$device ;;
  esac
  echo "  $SPOOLDIR/$base"
  if [ $dualqueue ]; then
    echo "  $SPOOLDIR/$base/raw"
  fi
done
) | sort -u

echo "
        + + + "
