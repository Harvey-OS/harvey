{
	verb[$4] = $3
	data[$4] = sprintf("%s %s %s %s %s %s", $5, $6, $7, $8, $9, $10)
}

END{
	for(i in verb)
		if(verb[i] != "d")
			printf("a %s %s\n", i, data[i]) |"sort +1"
	close("sort +1")
	for(i in verb)
		if(verb[i] == "d")
			printf("d %s %s\n", i, data[i]) |"sort -r +1"
	close("sort -r +1")
}
