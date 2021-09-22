.G1
frame invis bot solid
label bot "Populations (in Millions) of the 50 States"
label left "Number" "of" "States" left .3
ticks bot out from 0 to 25 by 5
coord x 0,25 y 0,13
copy "states2.d" thru X
  line from $1,0 to $1,$2
  line from $1,$2 to $1+1,$2
  line from $1+1,$2 to $1+1,0
X
.G2
