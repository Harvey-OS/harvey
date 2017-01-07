load('//$ARCH/include/harvey.BUILD', "harvey_binary")

harvey_binary(
    name="consoles",
    includes=[
        "//sys/include",
        "//${ARCH}/include",
    ],
    deps=[
        "comconsole",
        "pipeconsole",
        "screenconsole",
    ],
)
harvey_binary(
	name="console",
	deps=[
		"//sys/src/libc:libc",
	],
	srcs=[
		"buffer.c",
		"comconsole.c",
		"passthrough.c",
		"serve9p.c",
		"utils.c",
    ],
)


harvey_binary(
	name="pipeconsole",
	deps=[
		"//sys/src/libc:libc",
	],
	srcs=[
		"buffer.c",
		"pipeconsole.c",
		"passthrough.c",
		"serve9p.c",
		"utils.c",
	],
)

harvey_binary(
	name="screenconsole",
	deps=[
		"//sys/src/libc:libc",
	],
	srcs=[
		"buffer.c",
		"cga.c",
		"screenconsole.c",
		"keyboard.c",
		"passthrough.c",
		"serve9p.c",
		"utils.c",
	],
)
