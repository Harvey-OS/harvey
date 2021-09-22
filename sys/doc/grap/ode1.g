.G1
frame ht 2.5 wid 2.5
coord x 0,1 y 0,1
label bot "Direction field is $y sup prime = x sup 2 / y$"
label left "$y= sqrt {(2x sup 3 +1)/3}$" right .3
ticks left in 0 at 0,1
ticks bot in 0 at 0,1
len=.04
for tx from .01 to .91 by .1 do {
  for ty from .01 to .91 by .1 do {
    deriv=tx*tx/ty
    scale=len/sqrt(1+deriv*deriv)
    line from tx,ty to tx+scale,ty+scale*deriv
  }
}
draw solid
for tx = 0 to 1 by .05 do {
  next at tx, sqrt((2*tx*tx*tx+1)/3)
}
.G2
