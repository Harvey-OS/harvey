#!/bin/sh

#
# Writes a PostScript program on standard output that builds a width
# table or typesetter description file. The program uses PostScript's
# print procedure, which means the table comes back on the printer's
# serial port. Try hardcopy if you don't have access to the port.
#

POSTBIN=/usr/lbin/postscript
POSTLIB=/usr/lib/postscript
FONTDIR=/usr/lib/font

PROLOGUE=$POSTLIB/trofftable.ps
DPOSTPROLOGUE=$POSTLIB/dpost.ps

COPYFILE=
HOSTFONTDIR=
DEVICE=
LIBRARY=
TEMPLATE=

SLOWDOWN=25
STARTCOMMENTS=256

NONCONFORMING="%!PS"
ENDPROLOG="%%EndProlog"
BEGINSETUP="%%BeginSetup"
ENDSETUP="%%EndSetup"
TRAILER="%%Trailer"

while [ -n "$1" ]; do
    case $1 in
	-C)  shift; COPYFILE="$COPYFILE $1";;
	-C*) COPYFILE="$COPYFILE `echo $1 | sed s/-C//`";;

	-F)  shift; FONTDIR=$1;;
	-F*) FONTDIR=`echo $1 | sed s/-F//`;;

	-H)  shift; HOSTFONTDIR=$1;;
	-H*) HOSTFONTDIR=`echo $1 | sed s/-H//`;;

	-L)  shift; PROLOGUE=$1;;
	-L*) PROLOGUE=`echo $1 | sed s/-L//`;;

	-S)  shift; LIBRARY=$1;;
	-S*) LIBRARY=`echo $1 | sed s/-S//`;;

	-T)  shift; DEVICE=$1;;
	-T*) DEVICE=`echo $1 | sed s/-T//`;;

	-c)  shift; STARTCOMMENTS=$1;;
	-c*) STARTCOMMENTS=`echo $1 | sed s/-c//`;;

	-o)  shift; OCTALESCAPES=$1;;		# only for Latin1 tables
	-o*) OCTALESCAPES=`echo $1 | sed s/-o//`;;

	-s)  shift; SLOWDOWN=$1;;
	-s*) SLOWDOWN=`echo $1 | sed s/-s//`;;

	-t)  shift; TEMPLATE=$1;;
	-t*) TEMPLATE=`echo $1 | sed s/-t//`;;

	-*)  echo "$0: illegal option $1" >&2; exit 1;;

	*)   break;;
    esac
    shift
done

if [ ! "$DEVICE" -a ! "$LIBRARY" ]; then
    echo "$0: no device or shell library" >&2
    exit 1
fi

if [ $# -ne 1 -a $# -ne 2 ]; then
    echo "$0: bad argument count" >&2
    exit 1
fi

if [ -d "$HOSTFONTDIR" -a -f "$HOSTFONTDIR/$1" ]; then
    COPYFILE="$COPYFILE $HOSTFONTDIR/$1"
fi

#
# Include the shell library and get the command used to build the table.
# Make awk call a separate library function??
#

. ${LIBRARY:-${FONTDIR}/dev${DEVICE}/shell.lib}

if [ $# -eq 1 ]
    then TEMPLATE=$1
    else TEMPLATE=${TEMPLATE:-R}
fi

CMD=`BuiltinTables | awk '$2 == template"" {
	if ( pname == "" )
		pname = $3
	printf "%s %s %s", $1, tname, pname
	exit 0
}' template="$TEMPLATE" tname="$1" pname="$2"`

if [ ! "$CMD" ]; then
    echo "$0: $TEMPLATE not found" >&2
    exit 1
fi

#
# Build the PostScript font table program.
#

echo $NONCONFORMING
cat $PROLOGUE
echo "/DpostPrologue 100 dict dup begin"
cat $DPOSTPROLOGUE
echo "end def"
echo $ENDPROLOG

echo $BEGINSETUP
cat ${COPYFILE:-/dev/null}
echo "/slowdown $SLOWDOWN def"
echo "/startcomments $STARTCOMMENTS def"
echo $ENDSETUP

$CMD

echo $TRAILER

