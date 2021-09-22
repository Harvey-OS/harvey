awk '
BEGIN	{ bzs=0; bw=1e6 }  # bin zero start; bin width
	{ count[int(($3-bzs)/bw)]++ }
END	{ for (i in count) print i, count[i] }
' <states.d | sort -n >states2.d
