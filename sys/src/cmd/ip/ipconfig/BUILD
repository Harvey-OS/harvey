load("//sys/src/FLAGS", "CMD_LINK_OPTS", "LIB_COMPILER_FLAGS")

CMD_DEPS = [
    "//sys/src/libString:libString",
    "//sys/src/lib9p:lib9p",
    "//sys/src/libndb:libndb",
    "//sys/src/libauth:libauth",
    "//sys/src/libbio:libbio",
    "//sys/src/libip:libip",
    "//sys/src/libc:libc"
]

cc_binary(
	name="ipconfig",
	copts=[
		"-c",
		"-std=c11",
		"-mcmodel=small",
		"-O0",
		"-mno-red-zone",
		"-ffreestanding",
		"-fno-builtin",
		"-Wall",
		"-Wno-missing-braces",
		"-Wno-parentheses",
		"-Wno-unknown-pragmas",
		"-g",
	],
	includes=[
		"//sys/include",
		"//amd64/include",
		"//sys/src/cmd/ip/ipconfig",
	],
	deps=CMD_DEPS,
	linkopts=[
		"-static",
		"-e_main",
	],
	srcs=[
		"ipv6.c",
		"main.c",
		"ppp.c",
	],
)
