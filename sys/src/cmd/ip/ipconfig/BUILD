load('//sys/src/harvey.BUILD', "harvey_binary")

CMD_DEPS = [
	"//sys/src/libString:libString",
	"//sys/src/lib9p:lib9p",
	"//sys/src/libndb:libndb",
	"//sys/src/libauth:libauth",
	"//sys/src/libbio:libbio",
	"//sys/src/libip:libip",
	"//sys/src/libc:libc"
]

harvey_binary(
	name="ipconfig",
	srcs=[
		"ipv6.c",
		"main.c",
		"ppp.c",
	],
	install="bin/ip/ipconfig",
	includes=[
		"//sys/include",
		"//amd64/include",
		"//sys/src/cmd/ip/ipconfig",
	],
	deps=CMD_DEPS,
)
