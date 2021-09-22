.G1
label left "Representatives" "to Congress" left .3
label bot "Population (Millions)"
coord x .3,30 y .8,50 log log
define PlotState X circle at ($3/1e6,$2) X
copy "states.d" thru PlotState
.G2
