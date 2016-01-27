load("//sys/src/FLAGS", "LIB_COMPILER_FLAGS", "CMD_DEPS", "CMD_LINK_OPTS")

cc_binary(
    name="dump9660",
    copts=LIB_COMPILER_FLAGS,
    includes=[
        "//sys/include",
        "//amd64/include",
    ],
    deps=CMD_DEPS,
    linkopts=CMD_LINK_OPTS,
    srcs=[
        "boot.c",
        "cdrdwr.c",
        "conform.c",
        "direc.c",
        "dump.c",
        "dump9660.c",
        "ichar.c",
        "jchar.c",
        "path.c",
        "plan9.c",
        "rune.c",
        "sysuse.c",
        "util.c",
        "write.c",
    ],
)
