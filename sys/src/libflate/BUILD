load('//sys/src/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "all",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
        ],
	srcs = [
		"deflate.c",
		"deflatezlib.c",
		"deflateblock.c",
		"deflatezlibblock.c",
		"inflate.c",
		"inflatezlib.c",
		"inflateblock.c",
		"inflatezlibblock.c",
		"flateerr.c",
		"crc.c",
		"adler.c"
	]
)
