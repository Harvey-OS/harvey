.G1
ticks left off
cury=0
barht=.7
copy "prof2.d" thru X
  line from 0,cury to $1,cury
  line from $1,cury to $1,cury-barht
  line from 0,cury-barht to $1,cury-barht
  "  $2" ljust at 0,cury-barht/2
  cury=cury-1
X
line from 0,0 to 0,cury+1-barht
bars=-cury
frame invis ht bars/3 wid 3
.G2
