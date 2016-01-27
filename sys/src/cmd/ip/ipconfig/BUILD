load("//sys/src/FLAGS", "CMD_LINK_OPTS", "LIB_COMPILER_FLAGS")

CMD_DEPS = [
    "//sys/src/libndb:libndb",
    "//sys/src/libbio:libbio",
    "//sys/src/libip:libip",
    "//sys/src/libc:libc"
]

cc_binary(
	name="ipconfig",
	copts=LIB_COMPILER_FLAGS,
	includes=[
		"//sys/include",
		"//amd64/include",
	],
	deps=CMD_DEPS,
	linkopts=CMD_LINK_OPTS,
	srcs=[
		"ipv6.c",
		"main.c",
		"ppp.c",
	],
)
