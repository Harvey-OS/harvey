load("//sys/src/harvey.BUILD","LIB_COMPILER_FLAGS","CMD_DEPS","harvey_binary")

cc_library(
	name = "libvs",
	copts = LIB_COMPILER_FLAGS,
	includes=[
		"//sys/include",
		"//amd64/include",
	],
	srcs = [
		"arena.c",
		"arenas.c",
		"bloom.c",
		"buildbuck.c",
		"clump.c",
		"config.c",
		"conv.c",
		"dcache.c",
		"disksched.c",
		"dump.c",
		"graph.c",
		"hdisk.c",
		"hproc.c",
		"httpd.c",
		"icache.c",
		"icachewrite.c",
		"ifile.c",
		"index.c",
		"lump.c",
		"lumpcache.c",
		"lumpqueue.c",
		"part.c",
		"png.c",
		"round.c",
		"score.c",
		"sortientry.c",
		"stats.c",
		"syncarena.c",
		"syncindex0.c",
		"trace.c",
		"unwhack.c",
		"utils.c",
		"unittoull.c",
		"whack.c",
		"xml.c",
		"zblock.c",
		"zeropart.c",
	],
)
DEPS = [
	":libvs",
]

VENTI_DEPS =DEPS +CMD_DEPS 

harvey_binary(
	name = "venti",
	srcs=[
		"venti.c"
	],
	deps =VENTI_DEPS,
)
