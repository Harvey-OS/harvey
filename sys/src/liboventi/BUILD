load('//sys/src/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "liboventi",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
        ],
	srcs = [
		"client.c",
		"debug.c",
		"errfmt.c",
		"fatal.c",
		"pack.c",
		"packet.c",
		"parsescore.c",
		"readfully.c",
		"rpc.c",
		"scorefmt.c",
		"server.c",
		"strdup.c",
		"zero.c",
		"plan9-io.c",
		"plan9-sha1.c",
		"plan9-thread.c"
	]
)
