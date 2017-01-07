load('//$ARCH/include/harvey.BUILD', "harvey_binary")

DISK_SRCS=[
	"format.c",
	"exsort.c",
	"mbr.c",
	"mkext.c",
	"mkfs.c",
        "partfs.c",
]

[harvey_binary(
	name=d[:-2],
	srcs=[d],
) for d in DISK_SRCS]
