#	input is (decimal) shift-jis, unicode
#
function kut(x,		l, h){
	l = x%256; h = int(x/256)
	# inscrutable crap to convert MS kanji to JIS 206 (S2J macro from kogure)
	l -= 31
	if(l > 96) l--
	h -= 129
	if(h > 94) h -= 64
	h = 2*h + 33
	if(l > 126){ h++; l -= 94; }
	return(h*100 + l - 3232)
}
BEGIN	{
		big = 8407;
		for(i = 0; i <big; i++) c[i] = -1
	}
	{
		if($1 < 256){
			next		# dunno what to do with these
		}
		i = kut($1)
		if((i < 101) || (i >= big)){
			nskip++
			# if(nskip < 10) print "skipping", $1, "kuten=" i > "/fd/2"
			next
		}
		$2 = 0+$2
		if($2 < 3*4096) next
		if(c[i] != -1){
			if(dup[i]) dup[i] = dup[i] "," $2
			else dup[i] = c[i] "," $2
		}
		c[i] = $2
		print i, $2
	}
END	{
		for(i in dup) print "k=" i ":", dup[i] > "jis.dups"
		if(nskip) print nskip, "out of range mappings skipped" > "/fd/2"
	}
