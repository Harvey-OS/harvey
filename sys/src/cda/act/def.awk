BEGIN {prefix = "e:"}
/^#/ {next}
/^$/ {next}
/^:/ {
	prefix = $2":"
	next
}
{
	print prefix"	"$1
	if ($2 == "#")
		print "	{TOPDOWN;}"
	else if ($2 == "$")
		print "	{cost.gate += 1000; TOPDOWN;}"
	else if ($2 == "<")
		print "	{cost.gate -= 1; TOPDOWN;}"
	else {
		print "\t{"
		if (substr($2,1,1) == "$") {
			print "\t\tcost.gate += 1000;"
			$2 = substr($2,2,length($2))
		}
		else if (substr($2,1,1) == "<") {
			print "\t\tcost.gate -= 1;"
			$2 = substr($2,2,length($2))
		}
		if ($2 != "") {
			printf "\t\tif ("
			n = split($2,x,":");
			for (i = 1; i <= n; i++) {
				split(x[i],y,"=");
				printf "!eq($%s$,$%s$)%s",y[1],y[2],(i==n)?")\n":" ||\n\t\t\t"
			}
			print "\t\t\tABORT;"
		}
		print "\t\t\TOPDOWN;\n\t}"
	}
	print "	= {"
	j = 0
	k = 0
	args = ""
	for (i = 5; i <= NF; i++) {
		if ($i == "#")
			continue;
		if (split($i,x,":") == 2) {
			$i = x[1]
			out[$i] = "\t\tpush("((x[2]=="GND")?"ZERO":"ONE")"); namepin(\""$i"\", 0);\n"
			k++
		}
		else if (split($i,x,"%") == 2) {
			$i = x[1]
			out[$i] = out[x[2]]
			j++
		}
		else {
			j++
			extra = $i == "CLK"
			out[$i] = "\t\ttDO($%"i-4-k"$); namepin(\""$i"\", "extra");\n"
		}
		args = out[$i]""args
	}
	printf	"%s",args
	args = ""
	for (i = 4; i <= NF; i++)
		if ($i != "#")
			args = args",\""$i"\""
#	if (alist[$3] == "")
#		alist[$3] = args
#	verbose = (alist[$3] != args)
#if (verbose) print $3,args > "foo"
	print "		func($$,\""$3"\","j+k""args");\n\t};\n"
}
