#!/bin/awk -f
BEGIN {
	nsys = 0
}

/^#/ {
	next
}

{
	i=$1
	if(nsys > i){
		print "BROKEN TABLE: "nsys" > "i
		exit
	}
	while(nsys < i){
		sysarg[nsys] = 0
		sysnam[nsys] = "nosys"nsys
		sysfun[nsys] = "sys_nosys"
		nsys++;
	}
	sysarg[nsys] = $2
	sysnam[nsys] = $3
	sysfun[nsys] = $4
	nsys++
}

END {
	print "static Linuxcall linuxcalltab[] = {"
	for(i=0; i<nsys; i++){
		print "	{	/* "i" */"
		print "		.name = \""sysnam[i]"\","
		print "		.func = "sysfun[i]","
		print "		.stub = fcall"sysarg[i]","
		print "	},"
	}
	print "};"
	print ""
}
