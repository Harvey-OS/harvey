BEGIN {
	if (ARGV[1] == "-m") {
		stem = ARGV[2];
		ARGV[1] = ""
		ARGV[2] = ""
	}
	else {
		split(ARGV[ARGC-1], a, /\./)
		stem = a[1]
	}
	sym = 0
	pinfile = stem".netpin"
	tmpfile = "/tmp/"stem".xnf"
	print "LCANET, 1\nPROG, CDM2XNF\nPWR, 0, GND, 0\nPWR, 1, VCC, 1"
}
/^Pin/ {
	loc[$3] = $2
}
/^\.tt/ {
	ttline = ""
	for (i = 2; i <= NF; i++)
		ttline = ttline""$i
	tt[last] = ttline
}
/^\.t[ 	]/ {
	type[$2] = $3
	last = $2
}
/^\.f/ {
	split($2,x,".");
	stem = x[1]
}
/^\.c/ {
	if (part) {
		print "END" > tmpfile
		sym = 0
	}
	if ($2 == stem) {
		part = 0
		print "PART, "type[$3]
		print ".t",$3,type[$3] > pinfile
		print ".tt",tt[$3] > pinfile
	}
	else {
		part = 1
		print "SYM, "$2", "$3 > tmpfile
		sym = 1
	}
}
/^	/ {
	if (part) {
		io = ($3 == "O" || $3 == "Q") ? "O" : "I"
		print "PIN, "$3", "io", "$1 > tmpfile
		if (io == "O")
			output[$1] = 1
	}
	else {
		io = (output[$1]) ? "O" : "I"
		print "Net	"toupper($1)"	"$3"	"io > pinfile
#		print "Net	"$1"	"$3"	"io > pinfile
		print "EXT, "$1", "io",, LOC="loc[$2]
	}
}
END {
	system("cat "tmpfile)
	if (sym)
		print "END"
	print "EOF"
}
