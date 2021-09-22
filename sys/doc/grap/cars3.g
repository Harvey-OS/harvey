.G1
frame invis ht 3 wid 3
coord x 0,.10 y 0,5000
copy "cars.d" thru X
  tx=1/$1; ty=$2
  bullet at tx,ty
  tick bot at tx ""
  tick left at ty ""
X
.G2
