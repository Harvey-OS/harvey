function install(){lastf=$NF; last=$0; split($0,b,"\"")}
$NF!=lastf {if(last!="") print last; install()}
$NF==lastf {split($0,a,"\"")
	if(a[6]~/[-,]/||a[6]~/\(mi/) {
		split(a[6],aa,"[, ]")
		if(index(last,aa[1])==0) {
			print last
			install()
		}
		else if(b[6]!~/[,-]/&&b[6]!~/\(mi/)
			install()
	}
}
END {print last}
