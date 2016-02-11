load("//sys/src/FLAGS", "LIB_COMPILER_FLAGS", "CMD_DEPS")

cc_binary(
    name="factotum",
    copts=LIB_COMPILER_FLAGS,
    includes=[
        "//sys/include",
        "//amd64/include",
    ],
    deps=CMD_DEPS,
    srcs=[
        "apop.c",
        "chap.c",
        "confirm.c",
        "fs.c",
        "httpdigest.c",
        "log.c",
        "p9any.c",
        "p9cr.c",
        "p9sk1.c",
        "pass.c",
        "rpc.c",
        "rsa.c",
        "secstore.c",
        "util.c",
        "wep.c",
    ],
)
