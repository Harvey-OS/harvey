$NF!=lastf2 {if(last2!="") print last2
	 lastf2=lastf1; last2=last1; lastf1=$NF; last1=$0}
$NF==lastf2 {print}
END {print last2; print last1}
