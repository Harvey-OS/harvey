load('//sys/src/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "libsunrpc",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include"
            "//amd64/include"
        ],
	srcs = [
		"mount3.c",
		"nfs3.c",
		"portmap.c",
		"authunix.c",
		"client.c",
		"emalloc.c",
		"error.c",
		"fd.c",
		"fmt.c",
		"net.c",
		"prog.c",
		"rpc.c",
		"server.c",
		"suncall.c",
		"udp.c"
	]
)
