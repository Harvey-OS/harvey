.G1
frame ht 3 wid 3.5
label left "Population" "in Millions" "(Plotted as \(bu)"
label bot "Rank In Population" up .2
label right "Representatives" "(Plotted as \(sq)"
coord pop x 0,51 y .2,30 log y
coord rep x 0,51 y .3,100 log y
ticks left out at pop .3, 1, 3, 10, 30
ticks bot out at pop 1, 50
ticks right out at rep 1, 3, 10, 30, 100
thisrank=50
copy "states.d" thru X
  bullet at pop thisrank,$3/1e6
  square at rep thisrank,$2
  thisrank=thisrank-1
X
.G2
