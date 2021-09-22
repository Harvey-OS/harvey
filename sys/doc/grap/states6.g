.G1
frame invis bot solid left solid
label bot "Populations (in Millions) of the 50 States"
label left "Number" "of" "States" left .3
ticks bot out from 0 to 25 by 5
coord x 0,25 y 0,13
copy "states2.d" thru X
  line dotted from $1+.5,0 to $1+.5,$2
  "\(bu" size -3 at $1+.5,$2
X
.G2
