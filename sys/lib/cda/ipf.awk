BEGIN {
	fname = ARGV[1]
	if (fname == "/fd/0")
		fname = ARGV[2]
	split(fname,x,".")
	name = x[1]
	ipf = name".ipf"
	pins = name".pins"
	print "DEF "name"." > ipf
}
/Pin/ {
	tran[$2] = $3
}
/^NET/ {
	split($2,x,";")
	split(x[1],y,"_")
	port = y[1]
	clk = substr(port,1,3) == "clk"
	if (clk)
		next
	print $0 > ipf
}
/ PIN:/ {
	split($1,x,":")
	i = substr(x[2],1,length(x[2])-1)	# goddam actel
#	print $0 > ipf
#	print "USE ; "port"; FIX." > ipf
	if (!clk)
		printf "  PIN:%s,\n  FIX:''.\n",i > ipf
	if (pga == 1)
		i = tran[i]
	pin[port] = i
	if (type[port] != "")
		tt = substr(tt,1,i-1)""type[port]""substr(tt,i+1,length(tt)-i)
}
/^use/ {
	cell = $2
	port = $3
	if (cell == "INBUF" || cell == "CLKBUF")
		type[port] = "i"
	else if (cell == "OUTBUF")
		type[port] = "2"
	else if (cell == "BIBUF")
		type[port] = "3"
	else if (cell == "TRIBUFF")
		type[port] = "4"
}
/FAM.*a1200/ {family = 1200}
/FAM.*a1000/ {family = 1000}
/VAR.*PACK/ {
	n = split($4,x,"/")
	split(x[n],y,".")
	package = toupper(y[1])
	print ".t	"name"	"package > pins
	if (family == 1000) {
		if (package == "LCC68")		# wow, only depends on the package!
			tt = "nnnvnnnnn nnnnggnnnnnvnnnvn nnnnngnnnnnvnnnnn nnnnngnnnngvnnnnn nnnnngnn"
		else if (package == "LCC84")
			tt = "nnnvnnnnnnn nnnnnnggnnnnnvvnnnnnn vnnnnnngnnnnnvnnnnnnn nnnnnnggnnnngvvnnnnnn nnnnnnngnn"
		else if (package == "PGA132") {
			pga = 1
			tt = "gnnnnnnnnnnnn nnnngnnngnnnn nnnngnvngnnnn nnn nvn nnn nngggn nnngnnnn nvvvvvvv nnnnnnng ngggnn nnn nvn ngn nnnngnvngnnnn nnnnnnnngnnnn nnnnnnnnnnnnn"
		}
	}
	else if (family == 1200) {
		if (package == "LCC84")
			tt = "nnnnngnnnnn gnnnnnnnnnvvnnnngnnnn nnnnnnnnnnvnnnnngnnnn nnnnnnnnngvvnnnngnnnn nnnnnnnnnv"
		else if (package == "PGA132") {
			pga = 1
			tt = "?nnnnnnnnnnnn nnnngnnngnnnn nnnngnvngnnnn nnn nvn nnn nngggn nnngnnnn nvvvvvvv nnnnnnng ngggnn nnn nvn ngn nnnngnvngnnnn nnnnnnnngnnnn nnnnnnnnnnnnn"
		}
	}
	else
		print "family non-member"
	split(tt,x," ")
	tt = x[1]
	for (i = 2; x[i] != ""; i++)
		tt = tt""x[i]
#	tt = x[1]x[2]x[3]x[4]x[5]	# plcc-specific
	n = length(tt)
	for (i = 1; i <= n; i++)
		if (substr(tt,i,1) == "v")
			vcc[nv++] = i
		else if (substr(tt,i,1) == "g")
			gnd[ng++] = i
}
END {
	print "END." > ipf
	print ".tt	"tt > pins
	printf ".tp	VCC[0-%d]	",nv-1 > pins
	for (i in vcc)
		printf	"%d ",vcc[i] > pins
	printf "\n" > pins
	printf ".tp	GND[0-%d]	",ng-1 > pins
	for (i in gnd)
		printf	"%d ",gnd[i] > pins
	printf "\n" > pins
	for (i in pin)
		print	".tp	"i"	"pin[i] > pins
}
