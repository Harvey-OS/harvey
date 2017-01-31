load('//sys/src/harvey.BUILD', "harvey_binary")

group(
	name="consoles",
	deps=[
		":comconsole",
		":pipeconsole",
		":screenconsole",
	],
)
harvey_binary(
	name="comconsole",
	deps=[
		"//sys/src/libc:libc",
	],
	install="bin/aux/comconsole",
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
	install="bin/aux/pipeconsole",
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
	install="bin/aux/screenconsole",
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
