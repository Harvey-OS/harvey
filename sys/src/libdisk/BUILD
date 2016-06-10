load('//sys/src/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "all",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
        ],
	srcs = [
		"disk.c",
		"proto.c",
		"scsi.c",
		"scsierrs.c"
	]
)
