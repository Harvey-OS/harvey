load('//sys/src/harvey.BUILD', "harvey_binary")
load('//sys/src/FLAGS', "CMD_LINK_OPTS")

CMD_DEPS = [
	"//sys/src/lib9p:lib9p",
	"//sys/src/libauth:libauth",
	"//sys/src/libauthsrv:libauthsrv",
	"//sys/src/libcomplete:libcomplete",
	"//sys/src/libcontrol:libcontrol",
	"//sys/src/libdisk:libdisk",
	"//sys/src/libdraw:libdraw",
	"//sys/src/libflate:libflate",
	"//sys/src/libframe:libframe",
	"//sys/src/libgeometry:libgeometry",
	"//sys/src/libhttpd:libhttpd",
	"//sys/src/libip:libip",
	"//sys/src/liblex:liblex",
	"//sys/src/libmach:libmach",
	"//sys/src/libmemlayer:libmemlayer",
	"//sys/src/libmemdraw:libmemdraw",
	"//sys/src/libmp:libmp",
	"//sys/src/libndb:libndb",
	"//sys/src/libplumb:libplumb",
	"//sys/src/libregexp:libregexp",
	"//sys/src/libsec:libsec",
	"//sys/src/libstdio:libstdio",
	"//sys/src/libString:libString",
	"//sys/src/libthread:libthread",
	"//sys/src/libusb:libusb",
	"//sys/src/libventi:libventi",
	"//sys/src/liboventi:liboventi",
	"//sys/src/libbio:libbio",
	"//sys/src/libc:libc",
]

harvey_binary(
	name="dns",
	srcs=[
		"convDNS2M.c",
		"convM2DNS.c",
		"dblookup.c",
		"dn.c",
		"dnarea.c",
		"dnnotify.c",
		"dnresolve.c",
		"dns.c",
		"dnserver.c",
		"dnudpserver.c"
	],
	linkopts=CMD_LINK_OPTS,
	install="bin/ndb/dns",
	includes=[
		"//sys/include",
		"//amd64/include",
		"//sys/src/cmd/ndb",
	],
	deps=CMD_DEPS,
)

CMD_SRCS = [
	"cs.c",
	"csquery.c",
	"dnsquery.c",
	"ipquery.c",
	"inform.c",
	"mkdb.c",
	"mkhash.c",
	"mkhosts.c",
	"query.c"
]


[harvey_binary(
	name=c[:-2],
	srcs=[c],
	install="bin/ndb/%s" % c[:-2],
	deps=CMD_DEPS,
	linkopts=CMD_LINK_OPTS,
) for c in CMD_SRCS]

group(
	name="cmds",
	deps=[
		":cs",
		":csquery",
		":dnsquery",
		":dns",
		":ipquery",
		":inform",
		":mkdb",
		":mkhash",
		":mkhosts",
		":query"
	],
)
