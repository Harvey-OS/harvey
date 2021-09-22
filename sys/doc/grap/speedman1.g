.G1
ticks bot out from 10 to 70 by 10 ""
ticks bot out at 0 "1900", 40 "1940", 80 "1980"
label bot "World Record One Mile Run"
label left "Time" "(seconds)" left .4
lastyear=99; lastrec=0
copy "speedman.d" thru X
  lastyear=min($1,lastyear)
  lastrec=max($2,lastrec)
  line from lastyear,lastrec to $1,lastrec
  lastyear=$1; lastrec=$2
X
line from lastyear,lastrec to 84,lastrec
.G2
