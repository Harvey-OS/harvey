load('//$ARCH/include/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "libauthsrv",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
        ],
	srcs = [
		"_asgetticket.c",
		"_asrdresp.c",
		"authdial.c",
		"convA2M.c",
		"convM2A.c",
		"convM2PR.c",
		"convM2T.c",
		"convM2TR.c",
		"convPR2M.c",
		"convT2M.c",
		"convTR2M.c",
		"nvcsum.c",
		"opasstokey.c",
		"passtokey.c",
		"readnvram.c"
	]
)
