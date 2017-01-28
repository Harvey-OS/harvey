load('//$ARCH/include/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "libString",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
        ],
	srcs = [
		"s_alloc.c",
		"s_append.c",
		"s_array.c",
		"s_copy.c",
		"s_getline.c",
		"s_grow.c",
		"s_memappend.c",
		"s_nappend.c",
		"s_parse.c",
		"s_putc.c",
		"s_rdinstack.c",
		"s_read.c",
		"s_read_line.c",
		"s_reset.c",
		"s_terminate.c",
		"s_tolower.c",
		"s_unique.c"
	]
)
