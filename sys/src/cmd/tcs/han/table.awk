NR == 1	{
		lab1 = $1
		lab2 = $2
		big = $3
		for(i = 0; i <big; i++) c[i] = -1
		dupfile = lab1 ".dups"
	}
NR > 1	{
		i = $1
		if(i >= big){
			print "skip", $0, "(>" big ")" > "/fd/2"
			nskip++
			next
		}
		if(c[i] != -1){
			if(dup[i]) dup[i] = dup[i] "," $2
			else dup[i] = c[i] "," $2
			duptab = duptab i ", " $2 ", "
			c[i] = -c[i]
		} else
			c[i] = $2
	}
END	{
		print "#include	\"" lab1 ".h\""
		print
		print "long tab" lab1 "[" lab2 "MAX] = {"
		for(i = 0; i < big; i++){
			if(c[i] < 0) printf "%6d,", c[i]
			else printf "0x%04.4x,", c[i]
			if(i%8 == 7) print
		}
		if(i%8) print
		print "};"
		print "long " lab1 "dups[] = { " duptab " 0 };\n"
		if(nskip) print nskip, "out of range mappings skipped" > "/fd/2"
		n = 0
		for(i in dup){ n++; print "k=" i ":", dup[i] > dupfile }
		if(n) print n, "dups on", dupfile > "/fd/2"
	}
