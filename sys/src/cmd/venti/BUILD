load("//sys/src/harvey.BUILD", "harvey_binary")
CMDS = [
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
    name=c[:-2],
    srcs=[c],
) for c in CMDS]
