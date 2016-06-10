load('//sys/src/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "all",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
        ],
	srcs = [
		"regcomp.c",
		"regerror.c",
		"regexec.c",
		"regsub.c",
		"regaux.c",
		"rregexec.c",
		"rregsub.c"
	]
)
