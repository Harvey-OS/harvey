awk '
NR==2, NR==11 { print $1, substr($NF,2) }
' <prof1.d >prof2.d
