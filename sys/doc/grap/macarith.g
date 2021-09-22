.G1
frame ht 1.5 wid 1.5
define square X ($1)*($1) X
define root { exp(log($1)/2) }
define P !
    times at i, square(i); i=i+1
    circle at j, root(j); j=j+5
!
i=1; j=5
P; P; P; P; P
.G2
