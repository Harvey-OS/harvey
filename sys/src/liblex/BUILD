load('//sys/src/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "liblex",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include"
            "//amd64/include"
        ],
	srcs = [
		"allprint.c",
		"main.c",
		"reject.c",
		"yyless.c",
		"yywrap.c"
	]
)
