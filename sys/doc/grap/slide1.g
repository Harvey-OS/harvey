.ps 14
.vs 18
.G1 4
frame ht 2 wid 2
label left "Response" "Variable" left .5
label bot "Factor Variable"
line from 0,0 to 1,1
line dotted from .5,0 to .5,1
define blob X "\v'.2m'\(bu\v'-.2m'" X
blob at 0,.5; blob at .5,.5; blob at 1,.5
.G2
.ps
.vs
