load('//sys/src/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "libhttpd",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
        ],
	srcs = [
		"alloc.c",
		"checkcontent.c",
		"date.c",
		"fail.c",
		"gethead.c",
		"hio.c",
		"httpfmt.c",
		"httpunesc.c",
		"lower.c",
		"okheaders.c",
		"parse.c",
		"parsereq.c",
		"query.c",
		"redirected.c",
		"unallowed.c",
		"urlfmt.c",
		"urlunesc.c"
	]
)
