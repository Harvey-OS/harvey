#
# Builds one or more font width tables or the typesetter description
# file on a PostScript printer. Assumes you have direct access to the
# printer's serial port. No arguments means build a standard collection
# of tables - usually the LaserWriter Plus set. See trofftable and the
# shell library files /usr/lib/font/dev*/shell.lib for more details.
#

set -e

POSTBIN=/usr/lbin/postscript
POSTLIB=/usr/lib/postscript
FONTDIR=/usr/lib/font

POSTIO=$POSTBIN/postio
TROFFTABLE=$POSTBIN/trofftable

BAUDRATE=
DEVICE=
LIBRARY=

while [ -n "$1" ]; do
    case $1 in
	-C)  shift; OPTIONS="$OPTIONS -C$1";;
	-C*) OPTIONS="$OPTIONS $1";;

	-F)  shift; FONTDIR=$1;;
	-F*) FONTDIR=`echo $1 | sed s/-F//`;;

	-H)  shift; OPTIONS="$OPTIONS -H$1";;
	-H*) OPTIONS="$OPTIONS $1";;

	-S)  shift; LIBRARY=$1;;
	-S*) LIBRARY=`echo $1 | sed s/-S//`;;

	-T)  shift; DEVICE=$1;;
	-T*) DEVICE=`echo $1 | sed s/-T//`;;

	-b)  shift; BAUDRATE=$1;;
	-b*) BAUDRATE=`echo $1 | sed s/-b//`;;

	-c)  shift; OPTIONS="$OPTIONS -c$1";;
	-c*) OPTIONS="$OPTIONS $1";;

	-l)  shift; LINE=$1;;
	-l*) LINE=`echo $1 | sed s/-l//`;;

	-s)  shift; OPTIONS="$OPTIONS -s$1";;
	-s*) OPTIONS="$OPTIONS $1";;

	-t)  shift; OPTIONS="$OPTIONS -t$1";;
	-t*) OPTIONS="$OPTIONS $1";;

	-?)  OPTIONS="$OPTIONS $1$2"; shift;;
	-?*) OPTIONS="$OPTIONS $1";;

	*)   break;;
    esac
    shift
done

if [ ! "$DEVICE" -a ! "$LIBRARY" ]; then
    echo "$0: no device or shell library" >&2
    exit 1
fi

LIBRARY=${LIBRARY:-${FONTDIR}/dev${DEVICE}/shell.lib}

#
# No arguments means build everything return by the AllTables function.
#

if [ $# -eq 0 ]; then
    . $LIBRARY
    set -- `AllTables`
fi

for i do
    SHORT=`echo $i | awk '{print $1}'`
    LONG=`echo $i | awk '{print $2}'`

    if [ "$LINE" ]
	then echo "==== Building table $SHORT ===="
	else echo "==== Creating table program $SHORT.ps ===="
    fi

    $TROFFTABLE -S$LIBRARY $OPTIONS $SHORT $LONG >$SHORT.ps

    if [ "$LINE" ]; then
	$POSTIO -t -l$LINE ${BAUDRATE:+-b${BAUDRATE}} $SHORT.ps >$SHORT
	rm -f $SHORT.ps
    fi
done

