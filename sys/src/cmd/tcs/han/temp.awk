function dec(x,		i, j, c){
	j = 0
	if(substr(x, 1, 2) == "0x") x = substr(x, 3)
	for(i = 1; i <= length(x); i++){
		c = substr(x, i, 1)
		j = j*16 + dig[c]
		if(dig[c] == "") print "bad hex:", x > "/fd/2"
	}
	return(j)
}
function jis(x,	font, glyph, g, f){
	f = int(x/256)-32
	g = x%256 -32
	return(f*100+g)
}
BEGIN	{
		dig["0"] = 0;dig["1"] = 1;dig["2"] = 2;dig["3"] = 3;
		dig["4"] = 4;dig["5"] = 5;dig["6"] = 6;dig["7"] = 7;
		dig["8"] = 8;dig["9"] = 9;dig["a"] = 10;dig["b"] = 11;
		dig["c"] = 12;dig["d"] = 13;dig["e"] = 14;dig["f"] = 15;
		dig["A"] = 10;dig["B"] = 11;
		dig["C"] = 12;dig["D"] = 13;dig["E"] = 14;dig["F"] = 15;
		nskip = 0
	}
	{
		x = jis(dec($1))
		if($2 == "*") next
		k = dec($2)
		printf "%d %d\t\t# gb=%x 0x%x\n", x, k, dec($1), k
	}
