BEGIN{
	m = "common"
	haveold = 0;
	while(getline <"/n/9fat/plan9-3e.ini" > 0){
		haveold = 1
		if($0 ~ /\[.*\]/){
			m = substr($0, 2, length($0)-2)
			continue
		}
		if(m=="menu" && $0 ~ /^menuitem=4e,/)
			continue
		a[m] = a[m] $0 "\n"
	}

	a["4e"] = ""
	while(getline <"/tmp/plan9.ini" > 0)
		a["4e"] = a["4e"] $0 "\n"

	if(a["menu"] == "" && haveold){
		a["menu"] = "menuitem=3e, Plan 9 Third Edition\n"
		a["3e"] = ""
	}

	if(a["common"] != ""){
		for(i in a)
			if(i != "4e" && i != "common" && i != "menu")
				a[i] = a["common"] a[i]
		delete a["common"]
	}

	bootdisk4e=ENVIRON["fs"]
	gsub("/dev/", "boot(args|disk|file)=local!#S/", bootdisk4e)

	if(!haveold)
		print a["4e"]
	else{
		print "[menu]"
		print "menuitem=4e, Plan 9 Fourth Edition"
		print a["menu"]
		print ""
		delete a["menu"]
	
		print "[4e]"
		print a["4e"]
		print ""
		delete a["4e"]
	
		for(i in a){
			# BUG: if rootdir is already there we should rewrite it 
			# sometimes into /3e/whatwasthere
			if(a[i] ~ bootdisk4e && !(a[i] ~ /rootdir=/))
				a[i] = "rootdir=/root/3e\n" a[i]
			print "[" i "]"
			gsub(/9fat!9pcdisk/, "9fat!9pc3e", a[i])
			print a[i]
			print ""
		}
	}
}
