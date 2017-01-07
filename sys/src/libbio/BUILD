load('//$ARCH/include/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "libbio",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
        ],
	srcs = [
		"bbuffered.c",
		"bfildes.c",
		"bflush.c",
		"bgetrune.c",
		"bgetc.c",
		"bgetd.c",
		"binit.c",
		"boffset.c",
		"bprint.c",
		"bputrune.c",
		"bputc.c",
		"brdline.c",
		"brdstr.c",
		"bread.c",
		"bseek.c",
		"bwrite.c",
		"bvprint.c"
	]
)
