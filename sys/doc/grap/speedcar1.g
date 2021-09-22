.G1
label bot "World Land Speed Record"
label left "Miles" "per" "Hour" left .4
ticks bot out from 10 to 70 by 10 ""
ticks bot out at 0 "1900", 40 "1940", 80 "1980"
firstrecord=1
copy "speedcar.d" thru {
  if firstrecord==1 then {
    firstrecord=0
  } else {
    line from lastyear,lastrec to $1,lastrec
  }
  lastyear=$1; lastrec=$2
}
line from lastyear,lastrec to 84,lastrec
.G2
