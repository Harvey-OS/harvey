.G1
label left "Heights in Feet" "(Median and" "fifth percentiles)"
label bot "Heights of Boys in U.S., ages 2 to 18"
cmpft = 30.48  # Centimeters per foot
minx = 1e12; maxx = -1e12
n = sigx = sigx2 = sigy = sigxy = 0
copy "boyhts.d" thru X
  line from $1,$2/cmpft to $1,$4/cmpft
  ty = $3/cmpft
  bullet at $1,ty
  n = n+1
  sigx = sigx+$1; sigx2 = sigx2+$1*$1
  sigy = sigy+ty; sigxy = sigxy+$1*ty
  minx = min(minx,$1); maxx = max(maxx,$1)
X
# Calculate least squares fit and draw it
  slope = (n*sigxy - sigx*sigy) / (n*sigx2 - sigx*sigx)
  inter = (sigy - slope*sigx) / n
  # print slope; print inter
  line from minx,slope*minx+inter to maxx,slope*maxx+inter
.G2
