load('//sys/src/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "libplumb",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include"
            "//amd64/include"
        ],
	srcs = [
		"event.c",
		"mesg.c",
		"plumbsendtext.c"
	]
)
