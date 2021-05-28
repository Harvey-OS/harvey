BEGIN {
	dict["_seek"] = "__SEEK"
	dict["_exits"] = "__EXITS"
}
/^#define/ {
	w = $2
	dict[tolower(w)] = "_" w
	next
}
{
	s = $0
	t = ""
	while (s != "" && match(s, "[a-zA-Z0-9_]+")) {
		w = substr(s, RSTART, RLENGTH)
		u = dict[w]
		if (u == "") u = w
		t = t substr(s, 1, RSTART - 1) u
		s = substr(s, RSTART + RLENGTH)
	}
	print t s
}
