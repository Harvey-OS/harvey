.G1
frame invis ht 4 wid 3 bot solid
ticks off
coord x .5,3.5 y 0,25
define Ht X "- $1,000 -" size -3 at 2,$1 X
Ht(5); Ht(10); Ht(15); Ht(20)
"Highest Point" "in 50 States" at 1,23
"Heights of" "219 Volcanoes" at 3,23
"Feet" at 2,21.5; arrow from 2,22.5 to 2,24
define box X #(x,min,25%,median,75%,max,minname,maxname)
  xc=$1; xl=xc-boxwidth/2; xh=xc+boxwidth/2
  y1=$2; y2=$3; y3=$4; y4=$5; y5=$6
  bullet at xc,y1
  "  $7" size -3 ljust at (xc,y1)
  line from (xc,y1) to (xc,y2)  # lo whisker
  line from (xl,y2) to (xh,y2)  # box bot
  line from (xl,y3) to (xh,y3)  # box mid
  line from (xl,y4) to (xh,y4)  # box top
  line from (xl,y2) to (xl,y4)  # box left
  line from (xh,y2) to (xh,y4)  # box right
  line from (xc,y4) to (xc,y5)  # hi whisker
  bullet at xc,y5
  "  $8" size -3 ljust at (xc,y5)
X
boxwidth=.3
box(1, .3, 2.0, 4.6, 11.2, 20.3, Florida, Alaska)
box(3, .2, 3.7, 6.5,  9.5, 19.9, Ilhanova, Guallatiri)
.G2
