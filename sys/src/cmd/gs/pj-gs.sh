#!/bin/sh

# PaintJet driver script for Ghostscript,
# created by Philippe-Andre Prindeville <philipp@res.enst.fr>

# PCL level 1 interface
#
#=======================================================================#
# OPTIONS RECOGNIZED: ( all may be preceded with a "-" )		#
#	NOTE: Options marked with a "*" before their descriptions	#
#	      are provided for backward compatibility with the		#
#	      former hp2225a, hp2227a and hp3630a printer models -	#
# 	      these models have become links to this model. Consult	#
#	      your printer reference manual to determine which		#
#	      options are valid for your particular printer.		#
#									#
# Horizontal Pitch Selection:						#
#	c  		compressed print mode				#
#	e  	      * expanded print pitch				#
#	10 	      * 10 cpi (Pica print pitch)			#
#			  (expanded compressed on thinkjet and quietjet)#
#	12 	      * 12 cpi (Elite print pitch)			#
#									#
# Print Quality Selection						#
#	q | lq 	      * near letter quality				#
#									#
# Font Selection							#
#	b | bold      * set font stroke weight to bold			#
#									#
# Output filtering: (Default Cooked)					#
#	r | raw		raw mode for plotting mode etc.			#
#									#
# Other:								#
#       nb		do not output banner page (to save paper)	#
#									#
#		NOTE: * = NOT OFFICIAL PCL LEVEL 1 OPTIONS, USE OF	#
#			  THESE OPTIONS MAY OR MAY NOT PRODUCE		#
#			  DESIRED RESULTS.				#
#=======================================================================# 

PATH="/bin:/usr/bin:/usr/lib:/usr/local/bin"
export PATH

# set up redirection of stderr
log=/usr/spool/lp/log
exec 2>>$log

# sec_class=`getconf SECURITY_CLASS`
sec_class=
if [ $? -ne 0 ]
then
        echo "getconf SECURITY_CLASS failed"
fi

# Save the arguments to the model
printer=`basename $0`

if [ "$sec_class" = "2" ]       # B1 Trusted System
then
	reqid=$1
	user=$2
	dev=$3
	title=$4
	copies=$5
	options=$6
else
	reqid=$1
	user=$2
	title=$3
	copies=$4
	options=$5
fi


# Definitions of functions used within this script
do_banner()
{
	# Print the standard header
	x="XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
	echo "$x\n$x\n$x\n$x\n"
	banner `echo $user`
	echo "\n"
	user=`pwget -n $user | line | cut -d: -f5`
	if [ -n "$user" ]
	then
		echo "User: $user\n"
	else
		echo "\n"
	fi
	echo "Request id: $reqid    Printer: `basename $0`\n"
	date
	echo "\n"
	if [ -n "$title" ]
	then
		banner "$title" 
	fi
	echo "\014\r\c"
}

# Set up interface
if [ -t 1 ]
then
	stty 9600 opost onlcr -parenb cs8 ixon -istrip clocal tab3 <&1 2>/dev/null
else
	slp -n -k 2>/dev/null
fi

# Handle disable and cancel traps.
trap "echo 'Terminated: $reqid'  >> $log; trap 15; kill -15 0; exit 0 " 15

# Set up printer default modes
echo "\033&k0S\c"		# reset pitches
echo "\033(s0B\033)s0B\c"	# reset stroke weights
echo "\033&d@\c"		# disable auto-underline
echo "\033&l6D\c"		# reset to 6 lpi
echo "\033(s0Q\c"		# reset print quality
echo "\033&v0S\c"		# reset color
echo "\033&k2G\c"		# Set line termination mode


# Determine which options have been invoked
pitch="def"
weight="def"
quality="def"
# outputmode="cooked"
outputmode="raw"
# banner="yes"
banner=

for i in $options
do
	case "$i" in
	-c | c)   # compressed print
		pitch="c";;

	-e | e)   # expanded print
		pitch="e";;

	-10 | 10) # pitch set to 10 cpi
		pitch="10";;

	-12 | 12) # pitch set to 12 cpi
		pitch="12";;

	-q | q | -lq | lq) # near letter quality
		quality=1;;

	-b | b | -bold | bold) # set font weight to bold
		weight=1;;

	r | raw) # raw mode for binary output to printer
		outputmode="raw";;

	-nb | nb) # do not output banner page
		banner="";;

	esac
done

shift; shift; shift; shift; shift

if [ "$sec_class" = "2" ]       # B1 Trusted System
then
	shift
	files="$*"
	Nofilter= Nolabel=
	set -- `getopt fl $options`
	if [ $? != 0 ]
	then
		exit 2
	fi

	for opt in $*
	do
	    shift
	    case $opt in
	      -f) Nofilter=$opt ;;
	      -l) Nolabel=$opt ;;
	      --) break ;;
	    esac
	done

	# Print the sensitivity label of the process
	echo "$x\n$x\n"
	/usr/lib/lpbanner -j $reqid -t "$title" -u $user -p PCL1 -n $printer -d $dev $files
	echo "\n$x\n$x"

else
	# Assume that the rest of the arguments are files
	files="$*"
	# print the banner if nb option not specified
	if [ -n "$banner" ]
	then
		do_banner
	fi
fi

# Print the spooled files
i=1
while [ $i -le $copies ]
do
		for file in $files
		do

			# If raw mode, turn off output processing,
			# set for no tab expansion
			# If cooked mode, uncomment the cooked case if it is 
			# desired not to print on the page perforations
			case "$outputmode" in
				raw)	if [ -t 1 ]
					then
						stty raw 9600 -opost -parenb cs8 ixon -istrip clocal tab0 <&1 2>/dev/null
					else
						slp -r 2>/dev/null
					fi
					echo "\033&k0G";;		# Reset line termination mode
			#	cooked)	echo "\033&l1L\r\c";;
			esac

			case "$pitch" in
				def);;
				c)	echo "\033&k2S\r\c";;
				e)	echo "\033&k1S\r\c";;
				10)	echo "\033&k3S\r\c";;
				12)	echo "\033&k0S\r\c"
					echo "\033&k4S\r\c";;
			esac

			case "$quality" in
				def);;
				*)	echo "\033(s${quality}Q\r\c";;
			esac

			case "$weight" in
				def)	echo "\033(s0B\033)s0B\r\c";;
				*)	echo "\033(s${weight}B\r\c";;
			esac

			if [ "$sec_class" = "2" ]	# B1 Trusted System
			then
				/usr/lib/lprcat $Nofilter $Nolabel $file PCL1 $user $dev
			else
				type=`file $file | sed 's/^[^:]*..//'`
				case "$type" in
				postscript*)
#
# We could do the following, but this would leave gs with a rather large
# image in memory for (possibly) several minutes.  Better to use and
# intermediate file, since cat is "lightweight"...
#
#					gs -q -sDEVICE=paintjet -r180 -sOutputFile=- -dDISKFONTS -dNOPAUSE - < $file 2>/tmp/sh$$

					gs -q -sDEVICE=paintjet -r180 -sOutputFile=/tmp/pj$$ -dDISKFONTS -dNOPAUSE - < $file 1>2
					cat /tmp/pj$$
					rm /tmp/pj$$
					needff=
					;;
				*)	cat "$file" 2>/tmp/sh$$
					needff=1
					;;
				esac

				if [ -s /tmp/sh$$ ]
				then
#				    cat /tmp/sh$$	# output any errors
				    cat /tmp/sh$$ 1>2	# output any errors
				fi
				rm -f /tmp/sh$$
				if [ $needff ]; then echo "\014\r\c"; fi
			fi

			echo "\033&k0S\r\c"		# reset pitches
			echo "\033(s0B\033)s0B\r\c"	# reset stroke weights
			echo "\033&d@\r\c"		# disable auto-underline
			echo "\033&l6D\r\c"		# reset to 6 lpi
			echo "\033(s0Q\c"		# reset print quality
			echo "\033&v0S\c"		# reset color
		done
		i=`expr $i + 1`
	done

# Insure all buffers are flushed to printer
if [ -t 1 ]
then
	stty 9600 opost onlcr -parenb cs8 ixon -istrip clocal tab3 <&1 2>/dev/null
fi

exit 0
