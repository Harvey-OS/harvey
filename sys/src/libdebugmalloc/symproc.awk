# there must be a better way than this
function hexdigittoi(s) {
	if(s == "0") return 0;
	if(s == "1") return 1;
	if(s == "2") return 2;
	if(s == "3") return 3;
	if(s == "4") return 4;
	if(s == "5") return 5;
	if(s == "6") return 6;
	if(s == "7") return 7;
	if(s == "8") return 8;
	if(s == "9") return 9;
	if(s ~ /[Aa]/) return 10;
	if(s ~ /[Bb]/) return 11;
	if(s ~ /[Cc]/) return 12;
	if(s ~ /[Dd]/) return 13;
	if(s ~ /[Ee]/) return 14;
	if(s ~ /[Ff]/) return 15;
	return -1;
}

function hextoi(s,		v,i) {
	v = 0
	for(i=1; i<=length(s); i++)
		v = v*16 + hexdigittoi(substr(s, i, 1));
	return v;
}

function sortsyms(		i,j) {
	for(i=2; i<nsym; i++) {
		for(j=i-1; j>=0; j--) {
			if(addr[j] > addr[j+1]) {
				ta = addr[j];
				tn = name[j];
				addr[j] = addr[j+1];
				name[j] = name[j+1];
				addr[j+1] = ta;
				name[j+1] = tn;
			}
		}
	}
	sorted = 1;
}

function fname(a,		v,i) {
	v = hextoi(a);
	for(i=nsym-1; i>=0; i--) {
		if(addr[i] < v)
			return name[i];
	}
	return "?";
}

function ismalloc(a,		v) {
	v=fname(a);
	if(v ~ /^(pool|_?malloc|realloc|free|logstack$)/)
		return 1;
	return 0;
}

$1 ~ /^[0-9a-fA-F]+$/ {
	if($2 != "T" && $2 != "t")
		next;

	name[nsym] = $3;
	addr[nsym++] = hextoi($1);
	sorted=0;
	next;
}

$1 == "leak" {
	if(sorted == 0)
		sortsyms();
	for(i=4;i<=NF; i++) {
		# check symbol, replace with "src(" $i ");" or ""
		if(ismalloc($i))
			$i = "";
		else
			$i = "src(0x" $i ");"
	}
	print;
}
