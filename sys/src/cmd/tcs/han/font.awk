function dec(x,		i, j){
	j = 0
	for(i = 1; i <= length(x); i++)
		j = j*16 + dig[substr(x, i, 1)]
	return(j)
}
BEGIN	{
		FS = "	"
		big = 40000
		cs[2]="ugl";cs[3]="948";cs[4]="jis";cs[5]="942"
		cs[6]="ax";cs[7]="ksc";cs[8]="944"
		dig["0"] = 0;dig["1"] = 1;dig["2"] = 2;dig["3"] = 3;
		dig["4"] = 4;dig["5"] = 5;dig["6"] = 6;dig["7"] = 7;
		dig["8"] = 8;dig["9"] = 9;dig["a"] = 10;dig["b"] = 11;
		dig["c"] = 12;dig["d"] = 13;dig["e"] = 14;dig["f"] = 15;
		dig["A"] = 10;dig["B"] = 11;
		dig["C"] = 12;dig["D"] = 13;dig["E"] = 14;dig["F"] = 15;
	}
	{
		for(i = 2; i <= 8; i++) if(length($i))
			print dec($i), dec($1) "#" $0 > (cs[i] ".font" )
	}
END	{
		for(i = 2; i <= 8; i++){
			x = "sort -n -o " cs[i] ".font " cs[i] ".font"
			system(x)
		}
	}
