.G1
graph Linear
	coord x 1785,1955 y 0,160
	label left "Population" "in Millions" left .2
	label right "Linear Scale," "Linear Fit"
	ticks bot off
	copy "usapop.d"
	define fit X 35 + 1.4*($1-1870) X
	line from 1850,fit(1850) to 1950,fit(1950)
graph Exponential with .Frame.n at Linear.Frame.s -(0,.05)
	coord x 1785,1955 y 3,160 log y
	label left "Population" "in Millions" left .2
	label right "Logarithmic Scale," "Exponential Fit"
	copy "usapop.d"
	define fit X exp(0.75 + .012*($1-1800)) X
	line from 1790,fit(1790) to 1920,fit(1920)
.G2
