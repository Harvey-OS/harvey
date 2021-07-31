function dec(x,		i, j, c){
	j = 0
	for(i = 1; i <= length(x); i++){
		c = substr(x, i, 1)
		j = j*16 + dig[c]
		if(dig[c] == "") print "bad hex:", x > "/fd/2"
	}
	return(j)
}
BEGIN	{
		FS = "	"
		dig["0"] = 0;dig["1"] = 1;dig["2"] = 2;dig["3"] = 3;
		dig["4"] = 4;dig["5"] = 5;dig["6"] = 6;dig["7"] = 7;
		dig["8"] = 8;dig["9"] = 9;dig["a"] = 10;dig["b"] = 11;
		dig["c"] = 12;dig["d"] = 13;dig["e"] = 14;dig["f"] = 15;
		dig["A"] = 10;dig["B"] = 11;
		dig["C"] = 12;dig["D"] = 13;dig["E"] = 14;dig["F"] = 15;
		nskip = 0
	}
	{
		x = dec($1)
		if(x < 16384) print "warning:" NR ":", $1, " < 0x4000" > "/fd/2"
		ch = $2
		if(ch == "*") next
		if(substr(ch, 1, 2) != "0-") next
		k = substr(ch, 3)+0
		r = int(k/100)
		c = k%100
		if((c < 1) || (c > 94))
			print "warning:" NR ": bad col", $2 > "/fd/2"
		if((r < 1) || (r > 95))
			print "warning:" NR ": bad row", $2 > "/fd/2"
		printf "%d %d\t\t# b%x(%s) 0x%x\n", k, x, (r+160)*256+c+160, ch, x
	}
