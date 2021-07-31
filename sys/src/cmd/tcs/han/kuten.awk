function kut(x){
	return(int(x/256)*100 + x%256 - 3232)
}
BEGIN	{
		big = 8407;
		for(i = 0; i <big; i++) c[i] = -1
	}
	{
		i = $1
		if(i >= big){
			print "skip", $0 > "/fd/2"
			nskip++
			next
		}
		if(c[i] != -1){
			if(dup[i]) dup[i] = dup[i] "," $2
			else dup[i] = c[i] "," $2
		}
		c[i] = $2
	}
END	{
		print "#include	\"kuten.h\""
		print
		print "long tabkuten[KUTENMAX] = {"
		for(i = 0; i < big; i++){
			if(c[i] < 0) printf "    -1,"
			else printf "0x%04x,", c[i]
			if(i%8 == 7) print
		}
		if(i%8) print
		print "};"
		if(nskip) print nskip, "out of range mappings skipped" > "/fd/2"
		n = 0
		for(i in dup){ n++; print "k=" i ":", dup[i] > "jis.dups"; }
		if(n) print n, "dups on jis.dups" > "/fd/2"
	}
