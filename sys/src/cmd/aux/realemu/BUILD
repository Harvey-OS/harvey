load('//$ARCH/include/harvey.BUILD', "harvey_binary")

harvey_binary(
	name = "realemu",
	includes=[
		"//amd64/include",
		"//sys/include",
		"//sys/src/cmd/aux/realemu",
	 	"//.",
	],
	srcs = [
		"arg.c",
		"debug.c",
		"decode.c",
		"fmt.c",
		"main.c",
		"pit.c",
		"xec.c",
	],
)