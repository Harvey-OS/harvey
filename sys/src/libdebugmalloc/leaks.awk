$1 == "#stack" {
	$1="";
	stackstr=$0;
	next;
}

$1 == "poolalloc" {
	used[$5] = FILENAME ":" NR ": " $0;
	alloc[$5] = $3;
	stack[$5] = stackstr;
}

$1 == "poolfree" {
	used[$3] = "";
	stack[$3] = "";
}

END {
	for(x in used) {
		if(used[x] != "")
			printf("leak %d stack %s\n", alloc[x], stack[x]);
	}
}
