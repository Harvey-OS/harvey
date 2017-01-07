load('//$ARCH/include/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "libframe",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
        ],
	srcs = [
		"frbox.c",
		"frdraw.c",
		"frdelete.c",
		"frinit.c",
		"frinsert.c",
		"frptofchar.c",
		"frselect.c",
		"frstr.c",
		"frutil.c"
	]
)
