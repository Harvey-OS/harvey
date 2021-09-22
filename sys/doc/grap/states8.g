.G1
frame ht 4 wid 5
label left "Rank in" "Population"
label bot "Population (Millions)"
label top "$log sub 2$ (Population)"
coord x .3,30 y 0,51 log x
define L % exp($1*log(2))/1e6 "$1" %
ticks bot out at .5, 1, 2, 5, 10, 20
ticks left out from 10 to 50 by 10
ticks top out at L(19), L(20), L(21), L(22), L(23), L(24)
thisy=50
copy "states.d" thru X
  "$1" size -4 at ($3/1e6, thisy)
  thisy=thisy-1
X
line dotted from 15.3,1 to .515,50
.G2
