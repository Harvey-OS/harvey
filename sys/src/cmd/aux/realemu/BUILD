load("//sys/src/FLAGS", "CMD_LINK_OPTS", "CMD_DEPS", "LIB_COMPILER_FLAGS")

cc_binary(
	name = "realemu",
	copts =LIB_COMPILER_FLAGS,
	includes=[
		"//amd64/include",
		"//sys/include",
		"//sys/src/cmd/aux/realemu",
	 	"//.",
	],
	deps=CMD_DEPS,
	linkopts=CMD_LINK_OPTS,
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