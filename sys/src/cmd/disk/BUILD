load("//sys/src/harvey.BUILD", "harvey_binary")
DISKS = [
	"format.c",
        "partfs.c",
        "exsort.c",
        "mbr.c",
        "mkext.c",
        "mkfs.c",

]

[harvey_binary(
    name=c[:-2],
    srcs=[c],
) for c in DISKS]