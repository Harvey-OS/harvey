awk '
	{ x+=$1; x2+=$1*$1; y+=$2; xy+=$1*$2 }
END	{ slope=(NR*xy-x*y)/(NR*x2-x*x); print (y-slope*x)/NR, slope }
' $*
