awk '
# Parse all commands
$1=="file"	{ fname=$2 }
$1=="log"	{ logtext=$0 }
$1=="symbol"	{ symtext=$2 }
$1=="y"		{ yfield=$2; ylabel=$3 }
$1=="x"		{ n++; xfield[n]=$2; xlabel[n]=$3 }
# Generate n graphs
END {
print ".G1"
for (i=1; i<=n; i++) {
    if (s!="") print "#"
    print "graph A" s
    s=" with .Frame.w at A.Frame.e +(.1,0)"
    print "frame ht " 5/n " wid " 5/n
    print "label bot \"" xlabel[i] "\""
    if (i==1) print "label left \"" ylabel "\""
    if (logtext!="") print "coord " logtext
    print "ticks off"
    print "copy " fname " thru X " symtext\
	" at " xfield[i] "," yfield " X"
}
print ".G2"
}' $1
