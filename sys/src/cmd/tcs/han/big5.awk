function dec(x,		i, j, c){
	j = 0
	for(i = 1; i <= length(x); i++){
		c = substr(x, i, 1)
		j = j*16 + dig[c]
		if(dig[c] == "") print "bad hex:", x > "/fd/2"
	}
	return(j)
}
function big5(x,	font, glyph, g, f){
	font = int(x/256)
	glyph = x-256*font;
	if(font >= 161 && font <= 249) f = font-161
	else print NR ": bad font:", font, " from char", x > "/fd/2"
	if(glyph >= 64 && glyph <= 126) g = glyph-64
	else if(glyph >= 161 && glyph <= 254) g = glyph-161 + 63
	else print NR ": bad glyph:", glyph, " from char", x > "/fd/2"
	return(f*157+g)
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
		ch = $3
		if(ch == "*") next
		k = big5(dec(ch))
		printf "%d %d\t\t# b%d(%s) 0x%x\n", k, x, k, ch, x
	}
