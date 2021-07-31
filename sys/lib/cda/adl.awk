BEGIN {
	print "DEF XOR3; Y, A, B, C."
	print "USE ADLIB:XOR; x0."
	print "USE ADLIB:XOR; x1."
	print "NET A; A, x0:A."
	print "NET B; B, x0:B."
	print "NET D; x0:Y, x1:B."
	print "NET C; C, x1:A."
	print "NET Y; x1:Y, Y."
	print "END."
}
/^use/ {
	if ($2 == "INBUF" && substr($3,1,3) == "clk")
		$2 = "CLKBUF"
	part[$3] = $2
}
/^net/ {
	if ($4 == "PADY") {
		port = $2"_"
		ports = ports", "port
		net[port] = ", "port", "$3":PAD"
		$4 = "Y"
	}
	if ($4 == "PAD") {
		net[$2] = ", "$2""net[$2]
		ports = ports", "$2
	}
	pin = $3":"$4
	if ($3 == "POWER")
		net[$2] = net[$2]"; "pin
	else
		net[$2] = net[$2]", "pin
}
END {
	split(FILENAME,x,".");
	sub(",",";",ports)
	print "DEF "x[1]""ports"."
	for (p in part)
		if ((n = part[p]) == "XOR3")
			print "USE "part[p]"; "p"."
		else
			print "USE ADLIB:"part[p]"; "p"."
	for (n in net) {
		m = net[n]
		if (substr(m,1,1) == ";")
			m = "; "m
		sub(",",";",m)
		print "NET "n""m"."
	}
	print "END."
}
