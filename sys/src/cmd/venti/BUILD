load('//$ARCH/include/harvey.BUILD', "harvey_binary")

VENTI_SRCS=[
	"copy.c",
	"read.c",
	"ro.c",
	"sync.c",
	"write.c",
	"devnull.c",
	"mkroot.c",
	"randtest.c",
	"readlist.c",
	"root.c",
]

[harvey_binary(
	name=v[:-2],
	srcs=[v],
) for v in VENTI_SRCS]
