.G1
coord x 38,85 y .8,10000 log y
label bot "U.S. Army Personnel"
label left "Thousands" left .3
draw of solid   # Officers Female
draw ef dashed  # Enlisted Female
draw om dotted  # Officers Male
draw em solid   # Enlisted Male
copy "army.d" thru X
  next of at $1,$3
  next ef at $1,$5
  next om at $1,$2
  next em at $1,$4
X
copy thru % "$1 $2" size -3 at 60,$3 % until "XXX"
Enlisted Men 1200
Male Officers 140
Enlisted Women 12
Female Officers 2.5
XXX
.G2
