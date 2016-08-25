load('//sys/src/FLAGS', "LIB_COMPILER_FLAGS", "CMD_LINK_OPTS")
load('//sys/src/harvey.BUILD', "harvey_binary")

CMD_SRCS = [
	"echo.c",
	"cat.c",
	"date.c",
	"bind.c",
]

[harvey_binary(
	name=c[:-2],
	srcs=[c],
) for c in CMD_SRCS]

harvey_binary(
	name="mount",
	srcs=[
		"mount.c"
	],
	deps=[
		"//sys/src/libauth:libauth",
		"//sys/src/libc:libc",
	],
)

harvey_binary(
	name="srv",
	srcs=[
		"srv.c"
	],
	deps=[
		"//sys/src/libauth:libauth",
		"//sys/src/libc:libc",
	]
)

harvey_binary(
	name="ls",
	srcs=[
		"ls.c"
	],
	deps=[
		"//sys/src/libbio:libbio",
		"//sys/src/libc:libc",
	],
)

harvey_binary(
	name="ps",
	srcs=[
		"ps.c"
	],
	deps=[
		"//sys/src/libbio:libbio",
		"//sys/src/libc:libc",
	],
)

harvey_binary(
	name="ed",
	srcs=[
		"ed.c"
	],
	deps=[
		"//sys/src/libregexp:libregexp",
		"//sys/src/libbio:libbio",
		"//sys/src/libc:libc",
	],
	includes=[
		"//sys/include",
		"//amd64/include",
		"//sys/src/libregexp",
	],
 )

harvey_binary(
	name="ratrace",
	srcs=[
		"ratrace.c"
	],
	deps=[
        "//sys/src/libthread:libthread",
        "//sys/src/libc:libc",
	]
)
