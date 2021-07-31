/^\.I/ {
	$1 = ".o"
	$2 = "not"$2"@i"
	print $0
	next
}
{print $0}
