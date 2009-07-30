#!/bin/awk -f
# makes a table of character sets from http://www.iana.org/assignments/character-sets
# and tcs.txt

BEGIN{
	if(ARGC != 3){
		print "Usage:  " ARGV[0] " charsets.txt  tcs.txt"
		exit 1
	}
	while(getline<ARGV[1]){
		if(/^Name:/){
			i = 0
			name=tolower($2)
			names[name] = name
			alias[name i] = name
			nalias[name] = ++i
			
		}
		if(/^Alias:/){
			a = tolower($2)
			if(a != "none"){
				names[a] = name
				alias[name i ] = a
				nalias[name] = ++i
			}
		}
	}
}
{
	tcs = $1
	if(tcs in names){
		name = names[tcs]
		for(i=0; i<nalias[name]; i++)
			print "\"" alias[name i] "\", \"" $2 "\","
	}
}
