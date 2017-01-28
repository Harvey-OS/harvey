load('//$ARCH/include/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "libgeometry",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
        ],
	srcs = [
		"arith3.c",
		"matrix.c",
		"qball.c",
		"quaternion.c",
		"transform.c",
		"tstack.c"
	]
)
