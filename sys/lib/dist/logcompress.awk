{
	verb[$4] = $3
	data[$4] = sprintf("%s %s %s %s %s %s", $5, $6, $7, $8, $9, $10)
}

END{
	for(i in verb)
		printf("%s %s %s\n", verb[i]=="d" ? verb[i] : "a", i, data[i]) |"sort +1"
}
