awk '
BEGIN	{ bzs=0; bw=1e6 }  # bin zero start; bin width
	{ thisbin=int(($3-bzs)/bw); print $1, thisbin, count[thisbin]++ }
' <states.d >states3.d
