#!/bin/sh
#
# Unix lpr filter. The default setup sends output directly to a pipe,
# which requires the Ghostscript process to fork, and thus may cause 
# small systems to run out of memory/swap space. An alternative strategy,
# based on a suggestion by Andy Fyfe (andy@cs.caltech.edu), uses a named
# pipe for output, which avoids the fork and can thus save a lot of memory.
#
# Unfortunately this approach can cause problems when a print job is aborted, 
# as the abort can cause one of the processes to die, leaving the process 
# at the other end of the pipe hanging forever.
#
# Because of this, the named pipe method has not been made the default,
# but it may be restored by commenting out the lines referring to
# 'gsoutput' and uncommenting the lines referring to 'gspipe'.
#

PBMPLUSPATH=/usr/local/pbmplus/bin
PSFILTERPATH=/usr/local/tran/sparc/lib
LOCALPATH=/usr/local/bin
X11HOME=/usr/local/X11R5

PATH=/bin:/usr/bin:/usr/ucb:/usr/etc
PATH=${PATH}\:${LOCALPATH}\:${PBMPLUSPATH}\:${PSFILTERPATH}
LD_LIBRARY_PATH=${X11HOME}/lib

export PATH LD_LIBRARY_PATH acctfile host user

user= host= acctfile=/dev/null

#
# Redirect stdout to stderr (for the logfile) and open a new channel
# connected to stdout for the raw data. This enables us to keep the
# raw data separate from programmed postscript output and error messages.
#
exec 3>&1 1>&2

#
# Get username and hostname from filter parameters
#
while [ $# != 0 ]
do  case "$1" in
  -n)	user=$2 ; shift ;;
  -h)	host=$2 ; shift ;;
  -*)	;;
  *)	acctfile=$1 ;;
  esac
  shift
done

#
# Get the filter, printer device and queue type (direct/indirect)
#
filter=`basename $0`
device=`dirname $0`
type=`dirname ${device}`
device=`basename ${device}`
type=`basename ${type}`

#
# Find the bpp, if specified
#
case "${device}" in
  *.24) device=`basename ${device} .24` ; bpp=24 ;;
  *.16) device=`basename ${device} .16` ; bpp=16 ;;
  *.8)  device=`basename ${device} .8`  ; bpp=8  ;;
  *.3)  device=`basename ${device} .3`  ; bpp=3  ;;
  *.1)  device=`basename ${device} .1`  ; bpp=1  ;;
  *)    bpp=1 ;;
esac

#
# Information for the logfile
#
lock=`dirname ${acctfile}`/lock
cf=`tail -1 ${lock}`
job=`egrep '^J' ${cf} | tail +2c`

echo "gsbanner: ${host}:${user}  Job: ${job}  Date: `date`"
echo "gsif: ${host}:${user} ${device}.${bpp} start - `date`"

#
# Set the direct or indirect output destinations
#
#gspipe=/tmp/gspipe.$$
#mknod ${gspipe} p

case "${type}" in
  direct)
		gsoutput="cat 1>&3" ;;
#		cat ${gspipe} 1>&3 & ;;
  indirect)
		gsoutput="lpr -P${device}.raw" ;;
#		cat ${gspipe} | lpr -P${device}.raw & ;;
esac

(
#
# Any setup required may be done here (eg. setting gamma for colour printing)
#
echo "{0.333 exp} dup dup currenttransfer setcolortransfer"

#
# The input data is filtered here, before being passed on to Ghostscript
#
case "${filter}" in
  gsif)	  cat ;;
  gsnf)	  psdit ;;
  gstf)	  pscat ;;
  gsgf)	  psplot ;;
  gsvf)	  rasttopnm | pnmtops ;;
  gsdf)	  dvi2ps -sqlw ;;
  gscf|gsrf) echo "${filter}: filter not available" 1>&2 ; exit 0 ;;
esac

#
# This is the postlude which does the accounting
#
echo "\
(acctfile) getenv
  { currentdevice /PageCount gsgetdeviceprop dup cvi 0 gt
    { exch (a) file /acctfile exch def
      /string 20 string def
      string cvs dup length dup
      4 lt
        { 4 exch sub
          { acctfile ( ) writestring } repeat
        } { pop } ifelse
      acctfile exch writestring
      acctfile (.00 ) writestring
      acctfile (host) getenv 
        { string cvs } { (NOHOST) } ifelse writestring
      acctfile (:) writestring
      acctfile (user) getenv
        { string cvs } { (NOUSER) } ifelse writestring
      acctfile (\n) writestring
      acctfile closefile
    } { pop } ifelse
  } if
quit"
) | gs -q -dNOPAUSE -sDEVICE=${device} -dBitsPerPixel=${bpp} \
		-sOutputFile=\|"${gsoutput}" -
#		-sOutputFile=${gspipe} -

rm -f ${gspipe}
#
# End the logfile entry
#
echo "gsif: end - `date`"

