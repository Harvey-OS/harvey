.G1
graph A
frame ht 1.66667 wid 1.66667
label bot "Male_Officers"
label left "Enlisted_Men"
coord log x log y
ticks off
copy "army.d" thru X "\s-3$1\s+3" at $2,$4 X
#
graph A with .Frame.w at A.Frame.e +(.1,0)
frame ht 1.66667 wid 1.66667
label bot "Female_Officers"
coord log x log y
ticks off
copy "army.d" thru X "\s-3$1\s+3" at $3,$4 X
#
graph A with .Frame.w at A.Frame.e +(.1,0)
frame ht 1.66667 wid 1.66667
label bot "Enlisted_Women"
coord log x log y
ticks off
copy "army.d" thru X "\s-3$1\s+3" at $5,$4 X
.G2
