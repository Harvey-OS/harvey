load('//sys/src/harvey.BUILD', "harvey_binary")

harvey_binary(
	name="grep",
	includes=[
		"//sys/include",
		"//amd64/include",
		"//sys/src/cmd/grep"
	],
	deps=[
		":grepyacc",
		"//sys/src/libbio:libbio",
		"//sys/src/libc:libc",
	],
	srcs=[
		"comp.c",
		"main.c",
		"sub.c",
		"x.tab.c"
	],
)

yacc(
	name="grepyacc",
	yaccopts=[
		"-d",
		"--file-prefix=x"
	],
	srcs=[
		"grep.y"
	],
	hdrs=[
		"x.tab.h"
	],
	exports=[
		"x.tab.c"
	]
)
