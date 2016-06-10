load('//sys/src/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "all",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
        ],
	srcs = [
		"cache.c",
		"client.c",
		"conn.c",
		"dial.c",
		"debug.c",
		"dtype.c",
		"entry.c",
		"fcall.c",
		"fcallfmt.c",
		"file.c",
		"hangup.c",
		"log.c",
		"mem.c",
		"packet.c",
		"parsescore.c",
		"queue.c",
		"root.c",
		"rpc.c",
		"scorefmt.c",
		"send.c",
		"server.c",
		"srvhello.c",
		"strdup.c",
		"string.c",
		"time.c",
		"version.c",
		"zero.c",
		"zeroscore.c"
	]
)
