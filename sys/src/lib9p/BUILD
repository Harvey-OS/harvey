load('//$ARCH/include/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "lib9p",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
        ],
	srcs = [
		"auth.c",
		"dirread.c",
		"fid.c",
		"file.c",
		"intmap.c",
		"listen.c",
		"mem.c",
		"req.c",
		"parse.c",
		"post.c",
		"rfork.c",
		"srv.c",
		"thread.c",
		"uid.c",
		"util.c"
	]
)
