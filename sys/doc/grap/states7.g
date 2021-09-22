.G1
frame invis wid 4 ht 2.5 bot solid
ticks bot out from 0 to 25 by 5
ticks left off
label bot "Populations (in Millions) of the 50 States"
coord x 0,25 y 0,13
copy "states3.d" thru X "$1" size -4 at $2+.5, $3+.5 X
.G2
