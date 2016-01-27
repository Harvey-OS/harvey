load('//sys/src/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "libmach",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
        ],
	srcs = [
		"6.c",
		"6obj.c",
		"8db.c",
		"access.c",
		"executable.c",
		"machdata.c",
		"map.c",
		"obj.c",
		"setmach.c",
		"swap.c",
		"sym.c"
	]
)
