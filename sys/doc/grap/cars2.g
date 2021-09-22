.ft H
.ps -2
.vs -2
.G1
frame ht 2.5 wid 2.5
label left "Weight" "(Pounds)" left .3
label bot "Gallons per Mile"
coord x 0,.10 y 0,5000
ticks left from 0 to 5000 by 1000
ticks bot from 0 to .10 by .02
copy "cars.d" thru X circle at 1/$1, $2 X
.G2
.vs +2
.ps +2
.ft
